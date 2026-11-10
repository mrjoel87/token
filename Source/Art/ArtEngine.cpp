#include "ArtEngine.h"
#include "../Util.h"
#include <algorithm>
#include <cmath>

namespace token::art
{
namespace
{
    bool isBand (Zone z) { return z == Zone::BandSky || z == Zone::BandMid || z == Zone::BandLow; }

    float overlap (juce::Rectangle<float> a, juce::Rectangle<float> b)
    {
        const auto i = a.getIntersection (b);
        return i.getWidth() * i.getHeight();
    }

    /** Symmetric figures that may be turned by the beat as a whole. */
    bool spins (const Spec& s)
    {
        return (s.flags & flagSpin) != 0 || (s.category == Category::Geometric && s.zone != Zone::Full);
    }
}

ArtEngine::ArtEngine()
{
    rng.setSeedRandomly();
    active.reserve (6);
    refillBag();
}

int ArtEngine::librarySize() const noexcept { return (int) library().size(); }

juce::String ArtEngine::currentName() const
{
    for (const auto& a : active)
        if (a.hero && ! a.leaving)
            return library()[(size_t) a.spec].name;
    return {};
}

float ArtEngine::Active::alpha() const noexcept
{
    const float in = fadeIn > 0.0f ? age / fadeIn : 1.0f;
    const float out = leaving ? 1.0f - leaveAge / juce::jmax (0.05f, fadeOut) : 1.0f;
    return smoothstep (juce::jmin (in, out, 1.0f));
}

void ArtEngine::startLeaving (Active& a, float fadeSeconds)
{
    a.leaving = true;
    a.leaveAge = 0.0f;
    a.fadeOut = fadeSeconds;
}

// =================================================================================================
//  Shuffle bag
// =================================================================================================
void ArtEngine::refillBag()
{
    const int n = (int) library().size();
    bag.clear();
    for (int i = 0; i < n; ++i)
        bag.push_back (i);

    for (int i = n - 1; i > 0; --i)
        std::swap (bag[(size_t) i], bag[(size_t) rng.nextInt (i + 1)]);

    std::stable_partition (bag.begin(), bag.end(), [this] (int idx)
    {
        return std::find (history.begin(), history.end(), idx) == history.end();
    });
}

float ArtEngine::fit (const Spec& s) const
{
    const auto& a = s.affinity;
    return 0.15f + a.quiet * profile[0] + a.bass * profile[1] + a.melodic * profile[2] + a.energetic * profile[3];
}

int ArtEngine::pick (const ArtContext& c, bool hero, bool dramatic)
{
    const auto& lib = library();

    for (int attempt = 0; attempt < 2; ++attempt)
    {
        std::vector<std::pair<size_t, float>> candidates;
        bool anyNonRare = false;

        for (size_t pos = 0; pos < bag.size() && candidates.size() < 10; ++pos)
        {
            const int idx = bag[pos];
            const auto& s = lib[(size_t) idx];

            if (hero && (s.flags & flagHero) == 0)                  continue;
            if (! hero && (s.flags & flagCompanion) == 0)           continue;
            if (s.zone == Zone::Full && ! hero)                     continue;

            bool alive = false;
            for (const auto& a : active)
                alive = alive || a.spec == idx;
            if (alive)
                continue;

            float w = fit (s);
            w *= w;
            if (dramatic)
                w *= 0.3f + 1.5f * s.affinity.energetic + 0.5f * s.affinity.bass;
            if ((s.flags & flagRare) != 0)
                w *= 0.12f;
            else
                anyNonRare = true;

            w *= 1.0f / (1.0f + 0.08f * (float) candidates.size());

            for (const auto& a : active)
                if (! a.leaving && lib[(size_t) a.spec].category == s.category)
                    w *= 0.35f;

            candidates.push_back ({ pos, w });
        }

        if (candidates.empty() || ! anyNonRare)
        {
            refillBag();
            continue;
        }

        float total = 0.0f;
        for (const auto& cand : candidates)
            total += cand.second;

        float r = rng.nextFloat() * total;
        size_t chosen = candidates.front().first;
        for (const auto& cand : candidates)
        {
            r -= cand.second;
            if (r <= 0.0f)
            {
                chosen = cand.first;
                break;
            }
        }

        const int idx = bag[chosen];
        bag.erase (bag.begin() + (std::ptrdiff_t) chosen);
        history.push_back (idx);
        while (history.size() > 14)
            history.erase (history.begin());
        juce::ignoreUnused (c);
        return idx;
    }
    return -1;
}

// =================================================================================================
//  Composition: everything stays inside the safe canvas
// =================================================================================================
juce::Rectangle<float> ArtEngine::place (const Spec& s, const ArtContext& c)
{
    const auto A = c.area;
    if (s.zone == Zone::Full)
        return A;

    const float scale = 0.9f + rng.nextFloat() * 0.2f;
    const float grow = c.artOnly ? 1.12f : 1.0f;

    if (isBand (s.zone))
    {
        const float h = juce::jmin (A.getHeight() * 0.55f, A.getHeight() * s.size * scale * grow);
        float y = A.getCentreY() - h * 0.5f;
        if (s.zone == Zone::BandSky) y = A.getY() + A.getHeight() * 0.03f;
        if (s.zone == Zone::BandLow) y = A.getBottom() - h;
        return { A.getX(), y, A.getWidth(), h };
    }

    // size from the height, keep the drawing's own aspect ratio, never exceed the canvas
    float h = A.getHeight() * s.size * scale * grow;
    float w = h * s.aspect;
    if (w > A.getWidth() * 0.92f) { w = A.getWidth() * 0.92f; h = w / s.aspect; }
    if (h > A.getHeight() * 0.98f) { h = A.getHeight() * 0.98f; w = h * s.aspect; }

    // major symbols and geometry sit in the centre - that is where the eye goes
    if (s.zone == Zone::Centre)
    {
        const float jx = (rng.nextFloat() - 0.5f) * juce::jmin (A.getWidth() - w, A.getWidth() * 0.06f);
        return juce::Rectangle<float> (w, h).withCentre ({ A.getCentreX() + jx, A.getCentreY() });
    }

    juce::Rectangle<float> best;
    float bestScore = 1.0e30f;
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        // prefer the thirds of the canvas rather than arbitrary positions
        const float thirds[] = { 0.2f, 0.33f, 0.5f, 0.67f, 0.8f };
        const float cx = A.getX() + A.getWidth() * thirds[rng.nextInt (5)];
        float x = cx - w * 0.5f;
        float y = A.getCentreY() - h * 0.5f;

        if (s.zone == Zone::Sky)      y = A.getY() + A.getHeight() * 0.04f;
        if (s.zone == Zone::Ground)   y = A.getBottom() - h;
        if (s.zone == Zone::Anywhere) y = A.getY() + (A.getHeight() - h) * (0.3f + 0.4f * rng.nextFloat());

        x = juce::jlimit (A.getX(), juce::jmax (A.getX(), A.getRight() - w), x);
        y = juce::jlimit (A.getY(), juce::jmax (A.getY(), A.getBottom() - h), y);
        const juce::Rectangle<float> r (x, y, w, h);

        float score = 0.0f;
        for (const auto& a : active)
            if (! a.leaving)
                score += overlap (r, a.box);
        if (score < bestScore)
        {
            bestScore = score;
            best = r;
        }
    }
    return best;
}

void ArtEngine::spawn (int specIndex, const ArtContext& c, bool hero, bool dramatic)
{
    const auto& s = library()[(size_t) specIndex];
    Active a;
    a.spec = specIndex;
    a.hero = hero;
    a.life = (s.minLife + rng.nextFloat() * (s.maxLife - s.minLife)) * (1.0f + 0.35f * profile[0]) * (1.0f - 0.25f * profile[3]);
    a.fadeIn = dramatic ? 0.6f : 1.2f + rng.nextFloat() * 1.6f;
    a.fadeOut = 1.5f + rng.nextFloat() * 1.5f;
    a.flip = (s.flags & flagNoFlip) == 0 && rng.nextBool();
    a.entrance = isBand (s.zone) ? 0 : rng.nextInt (4);
    a.spinDir = rng.nextBool() ? 1.0f : -1.0f;
    a.seed = (juce::uint32) rng.nextInt (0x7fffffff);
    a.box = place (s, c);

    // a little drift, but never far enough to leave the canvas
    if (! isBand (s.zone) && s.zone != Zone::Full)
    {
        const auto A = c.area;
        const float roomL = (a.box.getX() - A.getX()) / juce::jmax (1.0f, A.getWidth());
        const float roomR = (A.getRight() - a.box.getRight()) / juce::jmax (1.0f, A.getWidth());
        a.driftX = juce::jlimit (-roomL, roomR, (rng.nextFloat() - 0.5f) * 0.05f);
        a.driftY = 0.0f;
    }

    active.push_back (a);
}

// =================================================================================================
//  Update
// =================================================================================================
void ArtEngine::update (const ArtContext& c)
{
    const float dt = c.dt;
    const auto* f = c.audio;
    if (f != nullptr)
        breakdown = f->breakdown;

    // musical clock: beat and bar crossings
    const double beatsNow = c.beats;
    double dBeats = beatsNow - lastBeats;
    if (firstUpdate || dBeats < 0.0 || dBeats > 2.0)     // first frame, loop or jump
        dBeats = 0.0;
    lastBeats = beatsNow;

    const auto beatIndex = (long long) std::floor (beatsNow);
    const auto barIndex = (long long) std::floor (beatsNow / juce::jmax (1.0, c.beatsPerBar));
    const bool beatCrossed = ! firstUpdate && beatIndex != lastBeatIndex;
    const bool barCrossed = ! firstUpdate && barIndex != lastBarIndex;
    lastBeatIndex = beatIndex;
    lastBarIndex = barIndex;
    firstUpdate = false;

    const float spd = c.speed * (1.0f - 0.35f * breakdown);
    const float bass = f != nullptr ? f->bass : 0.0f;

    for (auto& a : active)
    {
        a.age += dt;
        a.clock += dt * spd;
        a.travel += (float) dBeats * c.speed * (1.0f + 0.6f * bass);

        if (a.leaving)
        {
            a.leaveAge += dt;
            continue;
        }

        if (! c.enabled)
        {
            startLeaving (a, 0.8f);
            continue;
        }

        const bool held = c.locked && a.hero;
        if (! held && a.clock >= a.life)
            a.wantsLeave = true;
        if (held)
            a.wantsLeave = false;

        if (a.wantsLeave)
        {
            // leave on the next bar line; on slow material settle for the next beat
            a.waitTime += dt;
            if (barCrossed || (beatCrossed && a.waitTime > 2.5f) || a.waitTime > 6.0f)
                startLeaving (a, a.fadeOut);
        }
    }

    const bool heroWasAlive = std::any_of (active.begin(), active.end(), [] (const Active& a) { return a.hero && ! a.leaving; });
    active.erase (std::remove_if (active.begin(), active.end(), [] (const Active& a) { return a.finished(); }), active.end());

    if (! c.enabled || f == nullptr || c.area.getWidth() < 20.0f || c.area.getHeight() < 20.0f)
        return;

    // musical profile, slowly smoothed so the selection follows sections, not single notes
    const float onset = juce::jlimit (0.0f, 1.0f, f->onsetRate / 5.0f);
    const std::array<float, 4> target {
        juce::jlimit (0.0f, 1.0f, (1.0f - f->loudness) * 0.6f + f->breakdown * 0.6f + f->stateWeights[0] * 0.3f),
        juce::jlimit (0.0f, 1.0f, f->bassRel * 0.6f + f->bass * 0.3f - f->highRel * 0.15f),
        juce::jlimit (0.0f, 1.0f, f->sustain * 0.6f + f->midRel * 0.5f * (1.0f - onset)),
        juce::jlimit (0.0f, 1.0f, f->intensity * 0.6f + onset * 0.4f + f->drop * 0.5f)
    };
    for (size_t i = 0; i < profile.size(); ++i)
        profile[i] = smoothTo (profile[i], target[i], dt, 2.5f);

    // a drop: cut the current piece short and bring in something dramatic (not while locked)
    bool dramatic = false;
    if (f->drop > 0.95f && lastDrop <= 0.95f && ! c.locked)
    {
        for (auto& a : active)
            if (a.hero && ! a.leaving && a.age > 2.0f)
                startLeaving (a, 0.9f);
        heroTimer = 0.0f;
        dramatic = true;
    }
    lastDrop = f->drop;

    const bool heroAlive = std::any_of (active.begin(), active.end(), [] (const Active& a) { return a.hero && ! a.leaving; });
    if (heroWasAlive && ! heroAlive && ! dramatic)
        heroTimer = juce::jmap (profile[3], 3.0f, 0.6f) * (1.0f + 0.6f * profile[0]);   // short breath between pieces

    if (! heroAlive)
    {
        heroTimer -= dt * spd;
        const bool onTheBeat = beatCrossed || c.locked || dramatic;
        if ((heroTimer <= 0.0f || c.locked) && onTheBeat)
        {
            const int idx = pick (c, true, dramatic || f->drop > 0.6f);
            if (idx >= 0)
                spawn (idx, c, true, dramatic);
            heroTimer = 0.5f;
        }
    }

    if (c.locked)
        return;   // LOCK: no new companions either - just the chosen scene

    // small companions: birds, planes, comets, constellations, lanterns ...
    const int maxCompanions = c.artOnly ? 2 : 1;
    int companions = 0;
    for (const auto& a : active)
        if (! a.hero && ! a.leaving)
            ++companions;

    companionTimer -= dt * spd * (0.6f + 0.8f * profile[3] + 0.4f * profile[2]);
    if (companionTimer <= 0.0f && beatCrossed)
    {
        companionTimer = 5.0f + rng.nextFloat() * 8.0f;
        if (companions < maxCompanions && rng.nextFloat() < 0.8f)
        {
            const int idx = pick (c, false, false);
            if (idx >= 0)
                spawn (idx, c, false, false);
        }
    }
}

// =================================================================================================
//  Draw
// =================================================================================================
void ArtEngine::draw (juce::Graphics& g, const ArtContext& c, float alphaScale) const
{
    if (active.empty() || c.audio == nullptr || c.theme == nullptr || alphaScale <= 0.001f)
        return;

    const auto& f = *c.audio;
    const auto& lk = *c.theme;
    const auto& pal = c.palette;
    const float k = 0.3f + 1.2f * juce::jlimit (0.0f, 1.0f, c.intensity);

    Ink ink;
    ink.light = lk.light;
    if (lk.light)
    {
        ink.line   = lk.text.interpolatedWith (pal.a, 0.35f);
        ink.glow   = pal.b;
        ink.accent = pal.c;
        ink.sil    = juce::Colour (0xff1b1d22);
        ink.haze   = pal.b;
        ink.warm   = pal.c;
        ink.bg     = lk.screenTop;
    }
    else
    {
        ink.line   = pal.a.interpolatedWith (juce::Colours::white, 0.2f);
        ink.glow   = pal.b;
        ink.accent = pal.c;
        ink.sil    = lk.screenBottom.interpolatedWith (pal.b, 0.08f);
        ink.haze   = pal.b;
        ink.warm   = pal.c;
        ink.bg     = lk.screenBottom;
    }

    const float px = juce::jmax (1.0f, std::round (c.area.getHeight() / 110.0f));
    const float beats = (float) std::fmod (c.beats, 4096.0);
    const float beatsPerStep = c.speed >= 1.8f ? 0.5f : (c.speed >= 1.0f ? 1.0f : (c.speed >= 0.6f ? 2.0f : 4.0f));

    juce::Graphics::ScopedSaveState canvas (g);
    g.reduceClipRegion (c.area.expanded (2.0f).toNearestInt());

    for (const auto& a : active)
    {
        const auto& s = library()[(size_t) a.spec];
        float alpha = a.alpha() * alphaScale * (1.0f - 0.25f * breakdown);
        if (alpha <= 0.003f)
            continue;

        const bool held = c.locked && a.hero;
        const float cycle = juce::jmax (4.0f, 0.5f * (s.minLife + s.maxLife));

        Frame fr;
        fr.box = a.box;
        fr.t = a.clock;
        fr.life = held ? std::fmod (a.clock, cycle) / cycle : juce::jlimit (0.0f, 1.0f, a.clock / juce::jmax (0.1f, a.life));
        fr.px = px;
        fr.bass = juce::jlimit (0.0f, 1.0f, f.bass * k);
        fr.mid = juce::jlimit (0.0f, 1.0f, f.mid * k);
        fr.high = juce::jlimit (0.0f, 1.0f, f.high * k);
        fr.kick = juce::jlimit (0.0f, 1.0f, f.kick * k);
        fr.hat = juce::jlimit (0.0f, 1.0f, f.hat * k);
        fr.energy = f.intensity;
        fr.flow = f.sustain;
        fr.drop = f.drop;
        fr.beat = c.beatPhase;
        fr.beats = beats;
        fr.bar = c.barPhase;
        fr.travel = a.travel;
        fr.speed = c.speed;
        fr.spectrum = f.spectrum.data();
        fr.bands = AudioFeatures::numSpectrumBands;
        fr.seed = a.seed;
        fr.flip = a.flip;
        fr.ink = ink;

        juce::Graphics::ScopedSaveState state (g);
        const auto centre = a.box.getCentre();

        g.addTransform (juce::AffineTransform::translation (a.driftX * juce::jmin (1.0f, a.clock / juce::jmax (1.0f, a.life)) * c.area.getWidth(), 0.0f));

        // musical motion applied to the whole piece: kick pulse, bass swell, beat-stepped turning
        if (! isBand (s.zone) && s.zone != Zone::Full)
        {
            const bool geometric = spins (s);
            const float pulse = 1.0f + (geometric ? 0.07f : 0.025f) * fr.kick + (geometric ? 0.035f : 0.0f) * fr.bass;
            float angle = 0.0f;
            if (geometric)
                angle = a.spinDir * kit::beatStep (beats, beatsPerStep) * juce::MathConstants<float>::pi / 12.0f;
            g.addTransform (juce::AffineTransform::scale (pulse, pulse, centre.x, centre.y).rotated (angle, centre.x, centre.y));
        }

        // entrance: fade, wipe across, iris from the centre, or rise from below
        const float reveal = kit::ease (a.fadeIn > 0.0f ? a.age / a.fadeIn : 1.0f);
        if (reveal < 0.999f && ! a.leaving)
        {
            const auto b = a.box.expanded (6.0f);
            if (a.entrance == 1)
            {
                g.reduceClipRegion (b.withWidth (b.getWidth() * reveal * 1.1f).toNearestInt());
            }
            else if (a.entrance == 2)
            {
                juce::Path iris;
                const float r = (b.getWidth() + b.getHeight()) * 0.5f * reveal;
                iris.addEllipse (b.getCentreX() - r, b.getCentreY() - r, r * 2.0f, r * 2.0f);
                g.reduceClipRegion (iris);
            }
            else if (a.entrance == 3)
            {
                g.addTransform (juce::AffineTransform::translation (0.0f, (1.0f - reveal) * a.box.getHeight() * 0.06f));
            }
        }

        g.beginTransparencyLayer (juce::jmin (1.0f, alpha));
        s.draw (g, fr);
        g.endTransparencyLayer();
    }
}
}

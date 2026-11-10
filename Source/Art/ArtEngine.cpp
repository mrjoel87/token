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

// =================================================================================================
//  Shuffle bag
// =================================================================================================
void ArtEngine::refillBag()
{
    const int n = (int) library().size();
    bag.clear();
    for (int i = 0; i < n; ++i)
        bag.push_back (i);

    for (int i = n - 1; i > 0; --i)                         // Fisher-Yates
        std::swap (bag[(size_t) i], bag[(size_t) rng.nextInt (i + 1)]);

    // anything seen very recently goes to the back so a new cycle never starts with a repeat
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
            if (s.zone == Zone::Full && ! c.artOnly && ! hero)      continue;

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

            // slight preference for the front of the bag keeps the cycle moving
            w *= 1.0f / (1.0f + 0.08f * (float) candidates.size());

            // avoid two elements of the same category at once
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
        return idx;
    }
    return -1;
}

juce::Rectangle<float> ArtEngine::place (const Spec& s, const ArtContext& c)
{
    const auto A = c.area;
    if (s.zone == Zone::Full)
        return A;

    const float scale = rng.nextFloat() * 0.3f + 0.85f;
    const float grow = c.artOnly ? 1.15f : 1.0f;

    if (isBand (s.zone))
    {
        const float h = juce::jmin (A.getHeight() * 0.6f, A.getHeight() * s.size * scale * grow);
        float y = A.getCentreY() - h * 0.5f;
        if (s.zone == Zone::BandSky) y = A.getY() + A.getHeight() * (0.02f + 0.18f * rng.nextFloat());
        if (s.zone == Zone::BandLow) y = A.getBottom() - h - A.getHeight() * 0.04f * rng.nextFloat();
        return { A.getX(), y, A.getWidth(), h };
    }

    float h = A.getHeight() * s.size * scale * grow;
    float w = h * s.aspect;
    if (w > A.getWidth() * 0.95f) { w = A.getWidth() * 0.95f; h = w / s.aspect; }
    if (h > A.getHeight())        { h = A.getHeight();         w = h * s.aspect; }

    juce::Rectangle<float> best;
    float bestScore = 1.0e30f;
    for (int attempt = 0; attempt < 6; ++attempt)
    {
        const float x = A.getX() + rng.nextFloat() * juce::jmax (0.0f, A.getWidth() - w);
        float y = A.getY() + rng.nextFloat() * juce::jmax (0.0f, A.getHeight() - h);
        if (s.zone == Zone::Sky)    y = A.getY() + rng.nextFloat() * juce::jmax (0.0f, A.getHeight() * 0.5f - h * 0.6f);
        if (s.zone == Zone::Ground) y = A.getBottom() - h - rng.nextFloat() * A.getHeight() * 0.03f;
        const float xc = s.zone == Zone::Centre ? A.getCentreX() - w * 0.5f + (rng.nextFloat() - 0.5f) * A.getWidth() * 0.3f : x;
        if (s.zone == Zone::Centre) y = A.getCentreY() - h * 0.5f + (rng.nextFloat() - 0.5f) * A.getHeight() * 0.1f;

        const juce::Rectangle<float> r (juce::jlimit (A.getX(), juce::jmax (A.getX(), A.getRight() - w), xc), y, w, h);
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
    a.fadeIn = dramatic ? 0.8f : 1.8f + rng.nextFloat() * 2.2f;
    a.fadeOut = 2.0f + rng.nextFloat() * 2.0f;
    a.flip = (s.flags & flagNoFlip) == 0 && rng.nextBool();
    a.entrance = isBand (s.zone) ? 0 : rng.nextInt (4);
    a.driftX = isBand (s.zone) ? 0.0f : (rng.nextFloat() - 0.5f) * 0.06f;
    a.driftY = isBand (s.zone) ? 0.0f : (rng.nextFloat() - 0.5f) * 0.03f;
    a.seed = (juce::uint32) rng.nextInt (0x7fffffff);
    a.box = place (s, c);
    active.push_back (a);
}

// =================================================================================================
//  Update
// =================================================================================================
void ArtEngine::update (const ArtContext& c)
{
    const float dt = c.dt;

    for (auto& a : active)
    {
        a.age += dt;
        if (! a.leaving && (a.age >= a.life || ! c.enabled))
        {
            a.leaving = true;
            a.leaveAge = 0.0f;
            if (! c.enabled)
                a.fadeOut = 0.8f;
        }
        if (a.leaving)
            a.leaveAge += dt;
    }

    const bool heroWasAlive = std::any_of (active.begin(), active.end(), [] (const Active& a) { return a.hero && ! a.leaving; });
    active.erase (std::remove_if (active.begin(), active.end(), [] (const Active& a) { return a.finished(); }), active.end());

    if (! c.enabled || c.audio == nullptr || c.area.getWidth() < 20.0f || c.area.getHeight() < 20.0f)
        return;

    const auto& f = *c.audio;

    // musical profile, slowly smoothed so the selection follows sections, not single notes
    const float onset = juce::jlimit (0.0f, 1.0f, f.onsetRate / 5.0f);
    const std::array<float, 4> target {
        juce::jlimit (0.0f, 1.0f, (1.0f - f.loudness) * 0.6f + f.breakdown * 0.6f + f.stateWeights[0] * 0.3f),
        juce::jlimit (0.0f, 1.0f, f.bassRel * 0.6f + f.bass * 0.3f - f.highRel * 0.15f),
        juce::jlimit (0.0f, 1.0f, f.sustain * 0.6f + f.midRel * 0.5f * (1.0f - onset)),
        juce::jlimit (0.0f, 1.0f, f.intensity * 0.6f + onset * 0.4f + f.drop * 0.5f)
    };
    for (size_t i = 0; i < profile.size(); ++i)
        profile[i] = smoothTo (profile[i], target[i], dt, 2.5f);

    // a drop: cut the current piece short and bring in something dramatic
    bool dramatic = false;
    if (f.drop > 0.95f && lastDrop <= 0.95f)
    {
        for (auto& a : active)
            if (a.hero && ! a.leaving && a.age > 2.5f)
            {
                a.leaving = true;
                a.leaveAge = 0.0f;
                a.fadeOut = 1.2f;
            }
        heroTimer = juce::jmin (heroTimer, 0.4f);
        dramatic = true;
    }
    lastDrop = f.drop;

    const bool heroAlive = std::any_of (active.begin(), active.end(), [] (const Active& a) { return a.hero && ! a.leaving; });
    if (heroWasAlive && ! heroAlive && ! dramatic)
        heroTimer = juce::jmap (profile[3], 6.0f, 1.5f) * (1.0f + 0.6f * profile[0]);   // breathing room between pieces

    if (! heroAlive)
    {
        heroTimer -= dt;
        if (heroTimer <= 0.0f)
        {
            const int idx = pick (c, true, dramatic || f.drop > 0.6f);
            if (idx >= 0)
                spawn (idx, c, true, dramatic);
            heroTimer = 1.0f;
        }
    }

    // small companions: stars, birds, planes, lanterns ...
    const int maxCompanions = c.artOnly ? 2 : 1;
    int companions = 0;
    for (const auto& a : active)
        if (! a.hero && ! a.leaving)
            ++companions;

    companionTimer -= dt * (0.5f + 0.8f * profile[3] + 0.4f * profile[2]);
    if (companionTimer <= 0.0f)
    {
        companionTimer = 7.0f + rng.nextFloat() * 12.0f;
        if (companions < maxCompanions && rng.nextFloat() < 0.75f)
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
void ArtEngine::draw (juce::Graphics& g, const ArtContext& c) const
{
    if (active.empty() || c.audio == nullptr || c.theme == nullptr)
        return;

    const auto& f = *c.audio;
    const auto& lk = *c.theme;
    const auto& pal = c.palette;
    const float k = 0.3f + 1.2f * juce::jlimit (0.0f, 1.0f, c.intensity);

    Ink ink;
    ink.light = lk.light;
    if (lk.light)
    {
        ink.line   = lk.text.interpolatedWith (pal.a, 0.3f);
        ink.glow   = pal.b;
        ink.accent = pal.c;
        ink.sil    = juce::Colour (0xff1b1d22);
        ink.haze   = pal.b;
        ink.warm   = pal.c;
        ink.bg     = lk.screenTop;
    }
    else
    {
        ink.line   = pal.a.interpolatedWith (juce::Colours::white, 0.25f);
        ink.glow   = pal.b;
        ink.accent = pal.c;
        ink.sil    = lk.screenBottom.interpolatedWith (pal.b, 0.08f);
        ink.haze   = pal.b;
        ink.warm   = pal.c;
        ink.bg     = lk.screenBottom;
    }

    const float px = juce::jmax (1.0f, std::round (c.area.getHeight() / 110.0f));

    for (const auto& a : active)
    {
        const auto& s = library()[(size_t) a.spec];
        float alpha = a.alpha();
        if (! c.artOnly)
            alpha *= s.zone == Zone::Full ? 0.45f : 0.78f;
        if (alpha <= 0.003f)
            continue;

        Frame fr;
        fr.box = a.box;
        fr.t = a.age;
        fr.life = juce::jlimit (0.0f, 1.0f, a.age / juce::jmax (0.1f, a.life + a.fadeOut));
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
        fr.seed = a.seed;
        fr.flip = a.flip;
        fr.ink = ink;

        juce::Graphics::ScopedSaveState state (g);

        // gentle drift over the element's life
        g.addTransform (juce::AffineTransform::translation (a.driftX * fr.life * c.area.getWidth(), a.driftY * fr.life * c.area.getHeight()));

        // entrance: fade only, wipe across, iris from the centre, or rise from below
        const float reveal = kit::ease (a.fadeIn > 0.0f ? a.age / a.fadeIn : 1.0f);
        if (reveal < 0.999f && ! a.leaving)
        {
            const auto b = a.box.expanded (4.0f);
            if (a.entrance == 1)
            {
                g.reduceClipRegion (b.withWidth (b.getWidth() * reveal * 1.1f).toNearestInt());
            }
            else if (a.entrance == 2)
            {
                juce::Path iris;
                const float r = b.getWidth() * 0.5f + b.getHeight() * 0.5f;
                iris.addEllipse (b.getCentreX() - r * reveal, b.getCentreY() - r * reveal, r * 2.0f * reveal, r * 2.0f * reveal);
                g.reduceClipRegion (iris);
            }
            else if (a.entrance == 3)
            {
                g.addTransform (juce::AffineTransform::translation (0.0f, (1.0f - reveal) * a.box.getHeight() * 0.08f));
            }
        }

        g.beginTransparencyLayer (alpha);
        s.draw (g, fr);
        g.endTransparencyLayer();
    }
}
}

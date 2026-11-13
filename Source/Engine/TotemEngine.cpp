#include "TotemEngine.h"
#include "../Util.h"

namespace token::engine
{
namespace
{
    using Shape = std::array<float, SignalFrame::N>;

    /** Moving average (radius 8) with clamped edges. */
    void boxBlur (const Shape& src, Shape& dst)
    {
        const int last = SignalFrame::N - 1;
        const int radius = 8;
        float acc = 0.0f;
        for (int i = -radius; i <= radius; ++i)
            acc += src[(size_t) juce::jlimit (0, last, i)];
        for (int i = 0; i <= last; ++i)
        {
            dst[(size_t) i] = acc / (float) (radius * 2 + 1);
            acc += src[(size_t) juce::jmin (last, i + radius + 1)] - src[(size_t) juce::jmax (0, i - radius)];
        }
    }

    struct Ink
    {
        juce::Colour line, glow, accent;
    };

    /** Dark: luminous lines on black. Light: deep ink on paper. */
    Ink inkFor (const SignalFrame& s)
    {
        if (s.light)
            return { s.pal.b.darker (0.55f), s.pal.b, s.pal.c.darker (0.45f) };

        return { s.pal.a.interpolatedWith (juce::Colours::white, 0.1f + 0.3f * s.kick), s.pal.b, s.pal.c };
    }

    constexpr float drawOnSeconds = 1.6f;
    constexpr float dissolveSeconds = 1.4f;
}

TotemEngine::TotemEngine()
{
    lpScratch.resize ((size_t) ScopeHistory::size);
    negScratch.resize ((size_t) ScopeHistory::size);
    bag.reserve (glyphLibrary().size());
    rng.setSeedRandomly();
    summonHero();
}

const char* TotemEngine::currentName() const
{
    return hero.glyph >= 0 ? glyphLibrary()[(size_t) hero.glyph].name : "";
}

int TotemEngine::librarySize() const
{
    return (int) glyphLibrary().size();
}

// -------------------------------------------------------------------------------------------------
//  Signal extraction: a zero-crossing-triggered window, so periodic sound stands still
// -------------------------------------------------------------------------------------------------
void TotemEngine::extractTriggered (const ScopeHistory& h, float gain, float peak, double sr, int displayLen)
{
    const int n = SignalFrame::N;
    const auto total = h.getTotal();
    const auto oldest = h.oldest();
    const juce::int64 latestStart = total - displayLen - 1;
    const juce::int64 searchFrom = juce::jmax (oldest + 2, latestStart - displayLen);
    double startPos = (double) latestStart;

    if (latestStart > searchFrom + 8)
    {
        const int len = (int) juce::jmin ((juce::int64) lpScratch.size(), latestStart - searchFrom + 1);
        const float coeff = 1.0f - std::exp (-fx::twoPi * 250.0f / (float) sr);
        const float threshold = 0.08f * juce::jmax (1.0e-4f, peak);
        float lp = h.mono (searchFrom - 1);
        int lastNeg = -1;

        for (int i = 0; i < len; ++i)
        {
            lp += (h.mono (searchFrom + i) - lp) * coeff;
            lpScratch[(size_t) i] = lp;
            if (lp < -threshold)
                lastNeg = i;
            negScratch[(size_t) i] = lastNeg;
        }

        for (int i = len - 1; i >= 1; --i)
        {
            const float p0 = lpScratch[(size_t) (i - 1)], p1 = lpScratch[(size_t) i];
            const int neg = negScratch[(size_t) (i - 1)];
            if (p0 < 0.0f && p1 >= 0.0f && neg >= 0 && (i - neg) < displayLen / 2)
            {
                startPos = (double) (searchFrom + i - 1) + (double) (p0 / (p0 - p1));
                break;
            }
        }
    }

    const double step = (double) displayLen / (double) (n - 1);
    for (int c = 0; c < n; ++c)
    {
        const double pos = startPos + (double) c * step;
        const auto i0 = (juce::int64) std::floor (pos);
        const float fr = (float) (pos - (double) i0);
        const float v0 = (i0 < oldest || i0 >= total) ? 0.0f : h.mono (i0);
        const float v1 = (i0 + 1 < oldest || i0 + 1 >= total) ? 0.0f : h.mono (i0 + 1);
        sf.wave[(size_t) c] = std::tanh ((v0 + (v1 - v0) * fr) * gain);
    }
}

void TotemEngine::deriveShapes (float dt)
{
    const int n = SignalFrame::N;

    boxBlur (sf.wave, scratch);
    boxBlur (scratch, sf.low);

    for (int i = 0; i < n; ++i)
        sf.fine[(size_t) i] = sf.wave[(size_t) i] - sf.low[(size_t) i];

    const float k = first ? 1.0f : 1.0f - std::exp (-dt / (0.05f + 0.25f * sf.sustain));
    for (int i = 0; i < n; ++i)
        sf.slow[(size_t) i] += (sf.wave[(size_t) i] - sf.slow[(size_t) i]) * k;
}

// -------------------------------------------------------------------------------------------------
//  Summoning
// -------------------------------------------------------------------------------------------------
int TotemEngine::nextFromBag()
{
    if (bag.empty())
    {
        const int n = (int) glyphLibrary().size();
        for (int i = 0; i < n; ++i)
            bag.push_back (i);
        for (int i = n - 1; i > 0; --i)
            std::swap (bag[(size_t) i], bag[(size_t) rng.nextInt (i + 1)]);

        // never repeat the image that is on screen right now
        if (n > 1 && bag.back() == hero.glyph)
            std::swap (bag.back(), bag.front());
    }

    const int pick = bag.back();
    bag.pop_back();
    return pick;
}

void TotemEngine::summonHero()
{
    if (hero.glyph >= 0)
    {
        leaving = hero;
        leavingAge = 0.0f;
    }

    hero.glyph = nextFromBag();
    hero.age = 0.0f;
    hero.seed = (juce::uint32) rng.nextInt (0x7fffffff);
    hero.life = 11.0f + rng.nextFloat() * 8.0f;
}

GlyphCtx TotemEngine::contextFor (const Summon& m) const
{
    GlyphCtx c;
    c.t = m.age;
    c.spin = spin + (float) (m.seed % 628u) * 0.01f;
    c.beatPhase = sf.beatPhase;
    c.kick = sf.kick;
    c.bass = sf.bass;
    c.high = sf.high;
    c.energy = sf.energy;
    c.volume = sf.volume;
    c.seed = m.seed;
    return c;
}

// -------------------------------------------------------------------------------------------------
void TotemEngine::update (const ScopeHistory& h, const AudioFeatures& f, const ColourEngine& colours,
                          const EngineSettings& set, juce::Rectangle<float> area, double sampleRate, float dt, float time)
{
    const double sr = juce::jmax (8000.0, sampleRate);
    const float react = juce::jlimit (0.0f, 1.0f, set.react);
    const float k = 0.3f + 1.2f * react;   // REACT scales how hard the music moves things

    // musical features, smoothed where they drive geometry
    sf.audio = &f;
    sf.bass    = smoothTo (sf.bass, juce::jlimit (0.0f, 1.0f, (0.5f * f.bass + 0.5f * f.bassRel) * k), dt, 0.04f, 0.25f);
    sf.mid     = smoothTo (sf.mid, juce::jlimit (0.0f, 1.0f, (0.5f * f.mid + 0.5f * f.midRel) * k), dt, 0.1f, 0.4f);
    sf.high    = smoothTo (sf.high, juce::jlimit (0.0f, 1.0f, f.highRel * k), dt, 0.02f, 0.2f);
    sf.kick    = juce::jlimit (0.0f, 1.0f, f.kick * k);
    sf.snare   = juce::jlimit (0.0f, 1.0f, f.snare * k);
    sf.hat     = juce::jlimit (0.0f, 1.0f, f.hat * k);
    sf.sustain = smoothTo (sf.sustain, f.sustain * (1.0f - juce::jlimit (0.0f, 1.0f, f.onsetRate / 4.0f)), dt, 0.8f, 1.2f);
    sf.quiet   = smoothTo (sf.quiet, juce::jlimit (0.0f, 1.0f, f.silence * 0.7f + f.breakdown * 0.6f + (1.0f - f.loudness) * 0.3f - 0.15f), dt, 1.2f, 0.4f);
    sf.energy  = smoothTo (sf.energy, juce::jlimit (0.0f, 1.0f, (f.intensity - 0.4f) * 1.7f + f.drop * 0.5f - f.breakdown * 0.6f), dt, 0.3f, 1.0f);
    sf.volume  = smoothTo (sf.volume, juce::jlimit (0.0f, 1.0f, f.loudness * (1.0f - f.silence)), dt, 0.08f, 0.5f);
    sf.build   = f.build;
    sf.drop    = f.drop;
    sf.breakdown = f.breakdown;
    sf.kickHit = f.kickHit;
    sf.snareHit = f.snareHit;
    sf.dropHit = f.drop > 0.8f && lastDrop <= 0.8f;
    lastDrop = f.drop;

    // clocks: evolution and spin freeze with LOCK; motion (breathing) never stops
    if (! set.locked)
    {
        evo += dt * set.speed * (1.0f + 0.5f * sf.energy);
        spin += dt * set.speed * (0.22f + 0.5f * sf.energy + 0.6f * sf.build);
    }
    motion += dt * set.speed;

    sf.evo = evo;
    sf.motion = motion;
    sf.beats = colours.beats();
    sf.beatValid = colours.beatValid();
    sf.beatPhase = colours.beatPhase();
    sf.barPhase = colours.barPhase();
    sf.beatsPerBar = colours.beatsPerBar();
    sf.speed = set.speed;
    sf.react = react;
    sf.locked = set.locked;
    sf.strip = set.strip;
    sf.light = set.light;
    sf.quality = set.quality;
    sf.time = time;
    sf.dt = dt;
    sf.area = area;
    sf.pal = colours.palette();

    // the waveform
    const int displayLen = (int) juce::jlimit (256.0, 8192.0, sr * (set.strip ? 0.05 : 0.04));
    extractTriggered (h, f.scopeGain, f.peak, sr, displayLen);
    deriveShapes (dt);
    first = false;

    // snares split the lines into colours; drops shatter the image into copies
    glitch = juce::jmax (glitch * std::exp (-dt / 0.14f), sf.snare);
    shatter = sf.dropHit ? 1.0f : shatter * std::exp (-dt / 2.2f);

    // the dimension flip: every 4 bars with the host, otherwise on a strong kick now and then
    flipClock += dt * set.speed;
    if (! set.locked)
    {
        bool doFlip = false;
        if (sf.beatValid)
        {
            const auto bar = (long long) std::floor (sf.beats / (sf.beatsPerBar * 4.0));
            doFlip = lastFlipBar >= 0 && bar != lastFlipBar;
            lastFlipBar = bar;
        }
        else
        {
            lastFlipBar = -1;
            doFlip = sf.kickHit && sf.kick > 0.5f && flipClock > 9.0f;
        }

        if (doFlip)
        {
            flipTarget += fx::pi;
            flipClock = 0.0f;
        }
    }
    flip = smoothTo (flip, flipTarget, dt, 0.22f);

    // the summoned images
    const float ageStep = set.locked ? 0.0f : dt * juce::jmax (0.4f, set.speed);
    hero.age += set.locked ? (hero.age < drawOnSeconds ? dt : 0.0f) : ageStep;

    if (leavingAge >= 0.0f)
    {
        leavingAge += dt;
        leaving.age += dt;
        if (leavingAge >= dissolveSeconds)
            leavingAge = -1.0f;
    }

    if (! set.locked)
    {
        const bool onBar = ! sf.beatValid || sf.barPhase < 0.06f;
        const bool expired = hero.age >= hero.life && onBar;
        const bool dropped = sf.dropHit && hero.age > 3.0f;
        if ((expired || dropped) && leavingAge < 0.0f)
            summonHero();

        // smaller images pop up around the main one
        satelliteGap -= dt * set.speed * (0.6f + 0.8f * sf.energy);
        for (auto& m : satellites)
        {
            if (m.glyph < 0)
                continue;
            m.age += ageStep;
            if (m.age >= m.life)
                m.glyph = -1;
        }

        if (satelliteGap <= 0.0f && sf.quiet < 0.8f)
        {
            for (size_t i = 0; i < satellites.size(); ++i)
            {
                auto& m = satellites[i];
                if (m.glyph >= 0)
                    continue;
                m.glyph = rng.nextInt ((int) glyphLibrary().size());
                if (m.glyph == hero.glyph)
                    m.glyph = (m.glyph + 1) % (int) glyphLibrary().size();
                m.age = 0.0f;
                m.life = 5.0f + rng.nextFloat() * 5.0f;
                m.seed = (juce::uint32) rng.nextInt (0x7fffffff);
                const int otherSlot = satellites[1 - i].glyph >= 0 ? satellites[1 - i].slot : -1;
                m.slot = rng.nextInt (4);
                if (m.slot == otherSlot)
                    m.slot = (m.slot + 1) % 4;
                break;
            }
            satelliteGap = 4.0f + rng.nextFloat() * 7.0f;
        }
    }

    transmissions.update (sf);
}

// -------------------------------------------------------------------------------------------------
void TotemEngine::drawSummon (juce::Graphics& g, const Summon& m, juce::Point<float> c, float R, bool isHero) const
{
    if (m.glyph < 0)
        return;

    const auto& glyph = glyphLibrary()[(size_t) m.glyph];
    const auto ink = inkFor (sf);
    const int q = isHero ? sf.quality : juce::jmax (0, sf.quality - 1);

    sketch.clear();
    glyph.draw (sketch, contextFor (m));

    // appear by being drawn on; satellites also fade out at the end of their life
    const float reveal = fx::ease (m.age / drawOnSeconds);
    const float tail = isHero ? 1.0f : fx::ease ((m.life - m.age) / 1.0f);

    // it breathes with the volume, pulses with the kick, and flips through itself
    const float breathe = 1.0f + 0.035f * std::sin (sf.motion * 1.3f) + 0.1f * sf.volume + 0.12f * sf.kick;
    float flipX = isHero ? std::cos (flip) : 1.0f;
    if (std::abs (flipX) < 0.03f)
        flipX = flipX < 0.0f ? -0.03f : 0.03f;

    const bool turns = glyph.realm == Realm::Occult || glyph.realm == Realm::Sacred;
    const float rot = turns ? sf.evo * 0.06f * ((m.seed & 1u) ? 1.0f : -1.0f) : 0.0f;

    const auto xf = juce::AffineTransform::rotation (rot)
                        .scaled (R * breathe * flipX, R * breathe)
                        .translated (c.x, c.y);

    PenStyle st;
    st.amp = R * (0.012f + 0.06f * (0.25f + sf.bass) * (0.4f + sf.react) + 0.05f * sf.kick) * (isHero ? 1.0f : 0.8f);
    st.cycles = glyph.cycles;
    st.phase = sf.motion * 0.12f;
    st.reveal = reveal;
    st.stepPx = q >= 2 ? 2.2f : 3.2f;
    st.maxPoints = isHero ? (q >= 2 ? 2600 : (q == 1 ? 1700 : 1000)) : 700;

    auto& path = isHero ? heroPath : workPath;
    pen.build (sketch, xf, st, sf, path);

    const float alpha = tail * (1.0f - 0.3f * sf.quiet);
    const auto core = (isHero ? ink.line : ink.accent).withAlpha (0.95f * alpha);

    if (isHero)
    {
        // dimensions: echoes of the image falling away into depth
        const int echoes = q >= 2 ? 4 : (q == 1 ? 2 : 0);
        const float drift = sf.evo * 0.3f - std::floor (sf.evo * 0.3f);
        for (int i = echoes - 1; i >= 0; --i)
        {
            const float z = ((float) i + drift) / (float) juce::jmax (1, echoes);
            const float scale = 1.0f + z * (0.9f + 0.5f * sf.energy);
            const float a = (1.0f - z) * (1.0f - z) * (0.1f + 0.3f * sf.energy + 0.2f * sf.kick) * alpha * reveal;
            if (a < 0.01f)
                continue;
            const auto exf = juce::AffineTransform::rotation (z * 0.25f * std::sin (sf.motion * 0.2f), c.x, c.y).scaled (scale, scale, c.x, c.y);
            g.setColour (ink.glow.withAlpha (a));
            g.strokePath (path, fx::pen (1.0f), exf);
        }

        // drops: the image shatters into a ring of copies
        if (shatter > 0.03f && q >= 1)
        {
            for (int i = 0; i < 6; ++i)
            {
                const float a = fx::twoPi * (float) i / 6.0f + sf.motion * 0.3f;
                const auto pos = fx::onCircle (c, R * (1.0f + 0.9f * (1.0f - shatter)), a);
                const auto sxf = juce::AffineTransform::scale (0.3f, 0.3f, c.x, c.y).translated (pos.x - c.x, pos.y - c.y);
                g.setColour (ink.accent.withAlpha (0.6f * shatter * alpha));
                g.strokePath (path, fx::pen (2.5f), sxf);
            }
        }

        // snares: the lines split into colours
        if (glitch > 0.05f)
        {
            const float d = (2.0f + R * 0.03f) * glitch;
            g.setColour (ink.accent.withAlpha (0.55f * glitch * alpha));
            g.strokePath (path, fx::pen (1.0f), juce::AffineTransform::translation (d, -d * 0.4f));
            g.setColour (ink.glow.withAlpha (0.55f * glitch * alpha));
            g.strokePath (path, fx::pen (1.0f), juce::AffineTransform::translation (-d, d * 0.4f));
        }
    }

    ScopePen::glow (g, path, core, ink.glow.withAlpha (alpha), isHero ? 1.4f + 0.8f * sf.bass : 1.1f,
                    0.5f + 0.5f * sf.energy + 0.4f * sf.kick, q);
}

void TotemEngine::draw (juce::Graphics& g) const
{
    const auto A = sf.area;
    if (A.getWidth() < 8.0f || A.getHeight() < 8.0f)
        return;

    const auto ink = inkFor (sf);
    const int q = sf.quality;
    const juce::Point<float> c = A.getCentre();
    const float R = sf.strip ? juce::jmin (A.getHeight() * 0.4f, A.getWidth() * 0.16f)
                             : juce::jmin (A.getHeight() * 0.34f, A.getWidth() * 0.3f);

    // 1. the spine: the plain oscilloscope trace everything grows from
    {
        spinePath.clear();
        const int n = q >= 2 ? 260 : 150;
        const float half = A.getHeight() * (sf.strip ? 0.3f : 0.12f) * (0.4f + sf.react);
        for (int i = 0; i < n; ++i)
        {
            const float u = (float) i / (float) (n - 1);
            const float x = A.getX() + u * A.getWidth();
            const float y = c.y - fx::sampleAt (sf.wave, u) * half;
            if (i == 0) spinePath.startNewSubPath (x, y); else spinePath.lineTo (x, y);
        }
        g.setColour (ink.glow.withAlpha (0.28f + 0.3f * sf.volume));
        g.strokePath (spinePath, fx::pen (1.0f));
    }

    // 2. breathing rings around the centre
    {
        sketch.clear();
        const float breath = 1.0f + 0.04f * std::sin (sf.motion * 1.3f) + 0.08f * sf.volume;
        sketch.circle ({ 0.0f, 0.0f }, 1.32f * breath, 96);
        if (q >= 1)
            sketch.polygon ({ 0.0f, 0.0f }, 1.5f * breath, 6 + (int) (sf.evo * 0.02f) % 4, -sf.evo * 0.04f);

        PenStyle st;
        st.amp = R * (0.01f + 0.05f * sf.bass + 0.04f * sf.kick);
        st.cycles = 12.0f;
        st.phase = -sf.motion * 0.1f;
        st.maxPoints = q >= 2 ? 900 : 500;
        pen.build (sketch, juce::AffineTransform::scale (R, R).translated (c.x, c.y), st, sf, workPath);
        ScopePen::glow (g, workPath, ink.glow.interpolatedWith (ink.line, 0.3f).withAlpha (0.5f + 0.3f * sf.kick),
                        ink.glow, 1.0f, 0.4f + 0.5f * sf.kick, q);
    }

    // 3. the image that is dissolving
    if (leavingAge >= 0.0f && leaving.glyph >= 0)
    {
        const float p = leavingAge / dissolveSeconds;
        sketch.clear();
        glyphLibrary()[(size_t) leaving.glyph].draw (sketch, contextFor (leaving));

        PenStyle st;
        st.amp = R * (0.03f + 0.35f * p * p);
        st.cycles = glyphLibrary()[(size_t) leaving.glyph].cycles * (1.0f + 2.0f * p);
        st.phase = sf.motion;
        st.stepPx = 3.0f;
        st.maxPoints = q >= 2 ? 1400 : 800;
        const float sc = R * (1.0f + 0.6f * p);
        pen.build (sketch, juce::AffineTransform::scale (sc, sc).translated (c.x, c.y), st, sf, workPath);
        g.setColour (ink.glow.withAlpha ((1.0f - p) * (1.0f - p) * 0.7f));
        g.strokePath (workPath, fx::pen (1.0f));
    }

    // 4. smaller images popping up around the centre
    for (const auto& m : satellites)
    {
        if (m.glyph < 0)
            continue;

        juce::Point<float> pos;
        float r;
        if (sf.strip)
        {
            const float xs[4] = { 0.1f, 0.27f, 0.73f, 0.9f };
            pos = { A.getX() + A.getWidth() * xs[m.slot & 3], c.y };
            r = juce::jmin (A.getHeight() * 0.26f, A.getWidth() * 0.06f);
        }
        else
        {
            const float xs[4] = { 0.15f, 0.85f, 0.15f, 0.85f };
            const float ys[4] = { 0.2f, 0.2f, 0.8f, 0.8f };
            pos = { A.getX() + A.getWidth() * xs[m.slot & 3], A.getY() + A.getHeight() * ys[m.slot & 3] };
            r = juce::jmin (A.getHeight(), A.getWidth()) * 0.1f;
        }
        drawSummon (g, m, pos, r, false);
    }

    // 5. the summoned image
    drawSummon (g, hero, c, R, true);

    // 6. messages from nowhere
    transmissions.draw (g, sf, ink.accent, ink.glow, ink.line);
}
}

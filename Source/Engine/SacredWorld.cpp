#include "SacredWorld.h"
#include "../Util.h"

namespace token::engine
{
namespace
{
    const int foldChoices[] = { 8, 6, 12, 7, 10, 5, 9 };
    constexpr int numFoldChoices = (int) (sizeof (foldChoices) / sizeof (foldChoices[0]));
}

void SacredWorld::update (const SignalFrame& s)
{
    const float dt = s.dt;

    // snare -> brief spoke flash; drop -> bloom; sustained sound -> petals unfold; quiet -> breathe
    spokeFlash = juce::jmax (spokeFlash * std::exp (-dt / 0.16f), s.snare);
    bloom = s.dropHit ? 1.0f : bloom * std::exp (-dt / 1.4f);
    unfold = smoothTo (unfold, juce::jlimit (0.0f, 1.0f, 0.25f + 0.6f * s.sustain + 0.25f * s.energy - 0.3f * s.quiet), dt, 1.2f, 2.0f);
    breathe = 0.5f + 0.5f * std::sin (s.motion * 0.9f);
    foldBlend = juce::jmin (1.0f, foldBlend + dt / 2.5f);

    if (s.locked)
        return;   // LOCK: the composition holds; audio reactions continue in draw()

    // slow rotation (faster as a build-up climbs), plus eased steps on the beat
    rotation += dt * s.speed * (0.02f + 0.06f * s.mid + 0.12f * s.build);

    if (s.beatValid)
    {
        stepTarget = (float) std::floor (s.beats / 2.0) * fx::pi / (float) folds;
    }
    else if (s.kickHit && s.kick > 0.35f)
    {
        stepTarget += fx::pi / (float) (folds * 2);   // no host grid: turn on real kicks only
    }
    step = smoothTo (step, stepTarget, dt, 0.12f);

    // new symmetry every 8 bars with the host, otherwise every ~24 s of evolution
    phraseClock += dt * s.speed;
    bool newPhrase = false;
    if (s.beatValid)
    {
        const auto phrase = (long long) std::floor (s.beats / (s.beatsPerBar * 8.0));
        newPhrase = lastPhrase >= 0 && phrase != lastPhrase;
        lastPhrase = phrase;
    }
    else if (phraseClock > 24.0f)
    {
        newPhrase = true;
    }

    if (newPhrase)
    {
        phraseClock = 0.0f;
        previousFolds = folds;
        foldCursor = (foldCursor + 1) % numFoldChoices;
        folds = foldChoices[foldCursor];
        foldBlend = 0.0f;
    }

    // the rune glyph is read from the waveform every 4 bars (or ~10 s)
    glyphClock += dt * s.speed;
    bool newGlyph = false;
    if (s.beatValid)
    {
        const auto bar = (long long) std::floor (s.beats / (s.beatsPerBar * 4.0));
        newGlyph = lastGlyphBar >= 0 && bar != lastGlyphBar;
        lastGlyphBar = bar;
    }
    else if (glyphClock > 10.0f)
    {
        newGlyph = true;
    }

    if (newGlyph)
    {
        glyphClock = 0.0f;
        for (size_t i = 0; i < glyph.size(); ++i)
        {
            const float v = s.wave[(i * 53 + 17) % (size_t) SignalFrame::N];
            glyph[i] = v > 0.18f ? 1 : (v < -0.18f ? -1 : 0);
        }
    }
}

void SacredWorld::draw (juce::Graphics& g, const SignalFrame& s, float alpha) const
{
    if (alpha <= 0.005f)
        return;

    const auto A = s.area;

    // a frieze of rosettes in the strip, one great rosette in the box
    const float aspect = A.getWidth() / juce::jmax (1.0f, A.getHeight());
    int count = 1;
    if (aspect > 1.9f)
    {
        count = juce::jlimit (1, 7, juce::roundToInt (aspect * 0.55f));
        if (count % 2 == 0)
            ++count;
    }

    const float cellW = A.getWidth() / (float) count;
    for (int i = 0; i < count; ++i)
    {
        const bool primary = i == count / 2;
        const float R = juce::jmin (A.getHeight() * 0.47f, cellW * (primary ? 0.6f : 0.48f)) * (primary ? 1.0f : 0.82f);
        const juce::Point<float> c (A.getX() + cellW * ((float) i + 0.5f), A.getCentreY());
        drawRosette (g, s, c, R, (i % 2 == 0) ? 1.0f : -1.0f, primary, alpha * (primary ? 1.0f : 0.7f));
    }
}

void SacredWorld::drawRosette (juce::Graphics& g, const SignalFrame& s, juce::Point<float> c, float R, float dir,
                               bool primary, float alpha) const
{
    const auto& pal = s.pal;
    const int q = primary ? s.quality : juce::jmax (0, s.quality - 1);
    const int k = folds;
    const float rot = (rotation + step) * dir;
    const float react = 0.35f + 0.9f * s.react;
    const float quietDim = 1.0f - 0.35f * s.quiet;
    const float a = alpha * quietDim * (foldBlend < 1.0f ? (0.55f + 0.45f * foldBlend) : 1.0f);
    const float pulse = 1.0f + 0.14f * s.kick + 0.22f * bloom + 0.03f * breathe * s.quiet;
    const int ppf = q >= 2 ? 40 : (q == 1 ? 26 : 16);

    // faint guide circles
    g.setColour (pal.b.withAlpha (0.08f * a));
    g.drawEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f, 0.8f);
    g.drawEllipse (c.x - R * 0.62f, c.y - R * 0.62f, R * 1.24f, R * 1.24f, 0.6f);

    // 1. bass ring: the low waveform folded around the circle
    ring.clear();
    const int M = k * ppf;
    for (int j = 0; j <= M; ++j)
    {
        const float th = (float) j / (float) M * fx::twoPi;
        const float u = fx::foldU (th, k);
        const float v = fx::sampleAt (s.low, u * 0.5f + 0.25f);
        const float r = R * 0.3f * pulse + v * R * 0.13f * react;
        const auto p = fx::onCircle (c, r, th + rot);
        if (j == 0) ring.startNewSubPath (p); else ring.lineTo (p);
    }
    ring.closeSubPath();
    fx::glow (g, ring, pal.b.interpolatedWith (pal.a, 0.4f).withAlpha (0.9f * a), pal.b.withAlpha (a), 1.3f, 0.6f + s.bass, q);

    // 2. petals whose edges are the waveform
    petals.clear();
    const float ri = R * 0.36f * pulse;
    const float L = R * (0.3f + 0.42f * unfold) * pulse;
    const int K = juce::jmax (8, ppf / 2);
    for (int p = 0; p < k; ++p)
    {
        const float axis = rot * 0.6f + (float) p / (float) k * fx::twoPi;
        const float sa = std::sin (axis), ca = std::cos (axis);
        auto at = [&] (float radial, float lateral)
        {
            return juce::Point<float> (c.x + sa * radial + ca * lateral, c.y - ca * radial + sa * lateral);
        };

        for (int side = 0; side < 2; ++side)
        {
            const float sign = side == 0 ? 1.0f : -1.0f;
            for (int j = 0; j <= K; ++j)
            {
                const float t = (float) j / (float) K;
                const float shape = 0.65f + 0.55f * fx::sampleAt (s.slow, t) * react;
                const float w = R * 0.085f * std::sin (fx::pi * t) * shape;
                const auto pt = at (ri + t * L, sign * w);
                if (j == 0) petals.startNewSubPath (pt); else petals.lineTo (pt);
            }
        }

        if (q >= 1 && s.hat > 0.05f)   // hi-hats draw a fine vein down the petal
        {
            for (int j = 0; j <= K; ++j)
            {
                const float t = (float) j / (float) K;
                const auto pt = at (ri + t * L, fx::sampleAt (s.fine, t) * R * 0.05f * s.hat * std::sin (fx::pi * t));
                if (j == 0) petals.startNewSubPath (pt); else petals.lineTo (pt);
            }
        }
    }
    fx::glow (g, petals, pal.a.withAlpha (0.85f * a), pal.b.withAlpha (a), 1.0f, 0.5f + 0.6f * s.energy, q);

    // 3. frame: polygon and star, brighter as a build-up climbs
    frame.clear();
    fx::polygon (frame, c, k, R * 0.93f, rot);
    fx::star (frame, c, k, juce::jmax (2, k / 2 - (k % 2 == 0 ? 1 : 0)), R * 0.93f, rot);
    if (s.build > 0.3f)
        fx::polygon (frame, c, k, R * 0.78f, -rot * 1.5f);
    fx::glow (g, frame, pal.a.withAlpha ((0.35f + 0.4f * s.build + 0.3f * spokeFlash) * a), pal.b.withAlpha (a),
              0.9f + 0.8f * spokeFlash, 0.4f + s.build, q);

    // 4. rune spokes - branches read from the waveform, flashing on snares
    spokes.clear();
    for (int p = 0; p < k; ++p)
    {
        const float an = rot + ((float) p + 0.5f) / (float) k * fx::twoPi;
        const auto base = fx::onCircle (c, R * 0.94f, an), tip = fx::onCircle (c, R * 1.08f, an);
        spokes.startNewSubPath (base);
        spokes.lineTo (tip);
        for (int b = 0; b < 3; ++b)
        {
            const int code = glyph[(size_t) ((p + b) % 6)];
            if (code == 0)
                continue;
            const float rr = R * (0.97f + 0.035f * (float) b);
            const auto m = fx::onCircle (c, rr, an);
            const float len = R * 0.035f;
            if (code > 0 || b == 1)
            {
                spokes.startNewSubPath (m);
                spokes.lineTo (fx::onCircle (c, rr + len, an + 0.06f));
            }
            if (code < 0 || b == 1)
            {
                spokes.startNewSubPath (m);
                spokes.lineTo (fx::onCircle (c, rr + len, an - 0.06f));
            }
        }
    }
    fx::glow (g, spokes, pal.c.withAlpha ((0.35f + 0.65f * spokeFlash) * a), pal.c.withAlpha (a), 0.9f, spokeFlash, q);

    // 5. high-frequency dust
    if (q >= 1)
    {
        const int dots = k * 4;
        for (int i = 0; i < dots; ++i)
        {
            const float tw = fx::hash (17u, i + (int) (s.motion * 6.0f));
            const float size = 0.8f + 2.2f * s.hat * tw + 0.8f * s.high;
            const auto p = fx::onCircle (c, R * 0.86f, rot * 0.5f + (float) i / (float) dots * fx::twoPi);
            g.setColour (pal.a.withAlpha (juce::jlimit (0.0f, 1.0f, (0.2f + 0.6f * s.high * tw) * a)));
            g.fillEllipse (p.x - size * 0.5f, p.y - size * 0.5f, size, size);
        }
    }

    // 6. the core
    juce::Path core;
    fx::polygon (core, c, k, R * 0.08f * (1.0f + 0.5f * s.kick), -rot);
    fx::halo (g, c, R * 0.22f * (1.0f + s.kick), pal.c.withAlpha ((0.15f + 0.35f * s.kick + 0.3f * bloom) * a));
    fx::glow (g, core, pal.c.withAlpha (0.9f * a), pal.c.withAlpha (a), 1.0f, 0.5f, q);
}
}

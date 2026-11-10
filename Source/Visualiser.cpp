#include "Visualiser.h"
#include "Util.h"
#include <cmath>

namespace token
{
namespace
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;
    constexpr float pi = juce::MathConstants<float>::pi;

    juce::PathStrokeType stroke (float width)
    {
        return juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    }
}

// =================================================================================================
//  Signal extraction
// =================================================================================================
void Visualiser::extractTriggered (const ScopeHistory& h, float gain, float peak, double sr,
                                   int displayLen, std::vector<float>& out, int n)
{
    out.resize ((size_t) n);
    const auto total = h.getTotal();
    const auto oldest = h.oldest();
    const juce::int64 latestStart = total - displayLen - 1;
    const juce::int64 searchFrom = juce::jmax (oldest + 2, latestStart - displayLen);
    double startPos = (double) latestStart;

    // rising zero-crossing of a low-passed copy, armed by a preceding negative swing:
    // keeps periodic sounds standing still instead of sliding across the screen
    if (latestStart > searchFrom + 8)
    {
        const int len = (int) (latestStart - searchFrom + 1);
        lpScratch.resize ((size_t) len);
        negScratch.resize ((size_t) len);

        const float coeff = 1.0f - std::exp (-twoPi * 250.0f / (float) sr);
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

    const double step = n > 1 ? (double) displayLen / (double) (n - 1) : 0.0;

    for (int c = 0; c < n; ++c)
    {
        const double pos = startPos + (double) c * step;
        const auto i0 = (juce::int64) std::floor (pos);
        const float fr = (float) (pos - (double) i0);
        const float v0 = (i0 < oldest || i0 >= total) ? 0.0f : h.mono (i0);
        const float v1 = (i0 + 1 < oldest || i0 + 1 >= total) ? 0.0f : h.mono (i0 + 1);
        out[(size_t) c] = std::tanh ((v0 + (v1 - v0) * fr) * gain);
    }
}

void Visualiser::pushTrail (const juce::Path& p)
{
    trails[(size_t) trailHead] = p;
    trailHead = (trailHead + 1) % maxTrails;
    trailCount = juce::jmin (trailCount + 1, maxTrails);
}

// =================================================================================================
//  Update: musical reactions + geometry
// =================================================================================================
void Visualiser::update (const ScopeHistory& h, const AudioFeatures& f, const VisualSettings& s,
                         juce::Rectangle<float> a, double sampleRate, float dt, float t)
{
    area = a;
    mode = s.mode;
    strip = s.strip;
    time = t;
    k = 0.3f + 1.2f * juce::jlimit (0.0f, 1.0f, s.intensity);
    const double sr = juce::jmax (8000.0, sampleRate);

    // kick -> pulse, bass -> weight, energy/drops -> split lines, hats -> fine detail,
    // sustained pads -> flow (longer trails, softer motion), breakdowns -> calm (thinner, simpler)
    pulse    = smoothTo (pulse, juce::jlimit (0.0f, 1.0f, f.kick * k), dt, 0.01f, 0.13f);
    weight   = smoothTo (weight, 0.5f * f.bass + 0.5f * f.bassRel, dt, 0.05f, 0.3f);
    const float energy = juce::jlimit (0.0f, 1.0f, (f.intensity - 0.5f) * 2.0f + f.drop * 0.7f - f.breakdown * 0.8f);
    split    = smoothTo (split, juce::jlimit (0.0f, 1.0f, energy * k), dt, 0.4f, 1.2f);
    detail   = smoothTo (detail, juce::jlimit (0.0f, 1.0f, (0.5f * f.highRel + 0.9f * f.hat) * k), dt, 0.02f, 0.2f);
    glow     = smoothTo (glow, juce::jlimit (0.0f, 1.0f, 0.35f + 0.65f * f.intensity + 0.3f * f.drop - 0.3f * f.breakdown), dt, 0.3f, 1.0f);
    flow     = smoothTo (flow, f.sustain * (1.0f - juce::jlimit (0.0f, 1.0f, f.onsetRate / 4.0f)), dt, 1.0f);
    calm     = smoothTo (calm, juce::jlimit (0.0f, 1.0f, f.stateWeights[0] * 0.7f + f.breakdown), dt, 1.0f);
    symmetry = smoothTo (symmetry, juce::jlimit (0.0f, 1.0f, f.intensity * k * 1.3f + f.drop * 0.6f - 0.45f), dt, 0.8f, 1.5f);
    rotation += dt * s.speed * (0.04f + 0.35f * f.mid * k + 0.15f * flow);

    for (size_t i = 0; i < peaks.size(); ++i)
        peaks[i] = juce::jmax (f.spectrum[i], peaks[i] - dt * 0.35f);

    if (mode != builtMode)
    {
        builtMode = mode;
        trailCount = 0;
        raw.clear();
        disp.clear();
    }

    current.clear();
    detailPath.clear();
    centre = a.getCentre();

    if (mode == VisualMode::None || a.getWidth() < 8.0f || a.getHeight() < 8.0f)
        return;

    const float gain = f.scopeGain;
    const float tau = 0.012f + 0.10f * flow + 0.05f * calm;
    const float blendK = 1.0f - std::exp (-dt / tau);

    auto settle = [this, blendK]
    {
        if (disp.size() != raw.size())
            disp = raw;
        else
            for (size_t i = 0; i < raw.size(); ++i)
                disp[i] += (raw[i] - disp[i]) * blendK;

        high.assign (raw.size(), 0.0f);
        for (size_t i = 1; i + 1 < raw.size(); ++i)
            high[i] = raw[i] - (raw[i - 1] + raw[i] + raw[i + 1]) / 3.0f;
    };

    if (mode == VisualMode::Classic)
    {
        const int cols = juce::jmax (64, (int) (a.getWidth() / 2.5f));
        const int len = (int) juce::jlimit (256.0, 8192.0, sr * (strip ? 0.05 : 0.04));
        extractTriggered (h, gain, f.peak, sr, len, raw, cols);
        settle();

        halfHeight = a.getHeight() * 0.42f * (1.0f + 0.16f * pulse);
        const float cy = a.getCentreY();
        const float dx = a.getWidth() / (float) (cols - 1);
        auto pt = [&] (int c) { return juce::Point<float> (a.getX() + (float) c * dx, cy - disp[(size_t) c] * halfHeight); };

        current.preallocateSpace (cols * 5);
        current.startNewSubPath (pt (0));
        for (int c = 1; c < cols - 1; ++c)
        {
            const auto p1 = pt (c), p2 = pt (c + 1);
            current.quadraticTo (p1, (p1 + p2) * 0.5f);
        }
        current.lineTo (pt (cols - 1));

        if (detail > 0.03f)
            for (int c = 0; c < cols; ++c)
            {
                const float y = cy - (disp[(size_t) c] + high[(size_t) c] * 5.0f * detail) * halfHeight;
                if (c == 0) detailPath.startNewSubPath (a.getX(), y);
                else        detailPath.lineTo (a.getX() + (float) c * dx, y);
            }

        // the signal's local envelope (for revealing the artwork through the waveform)
        if (env.size() != (size_t) cols)
            env.assign ((size_t) cols, 0.0f);
        for (int c = 0; c < cols; ++c)
        {
            float m = 0.0f;
            for (int j = juce::jmax (0, c - 4); j <= juce::jmin (cols - 1, c + 4); ++j)
                m = juce::jmax (m, std::abs (disp[(size_t) j]));
            env[(size_t) c] = smoothTo (env[(size_t) c], m, dt, 0.04f, 0.4f);
        }

        pushTrail (current);
    }
    else if (mode == VisualMode::Orbit)
    {
        const int n = 300, blend = 36;
        const double baseLen = juce::jlimit (256.0, 8192.0, sr * 0.028);
        extractTriggered (h, gain, f.peak, sr, (int) (baseLen * (double) (n + blend) / (double) n), raw, n + blend);
        settle();

        const float base = juce::jmin (a.getWidth(), a.getHeight()) * (strip ? 0.36f : 0.27f);
        if (radius <= 0.0f)
            radius = base;
        radius = smoothTo (radius, base * (1.0f + 0.10f * pulse + 0.07f * weight * k - 0.05f * calm), dt, 0.03f, 0.25f);
        orbitR = radius;
        orbitAmp = base * (0.30f + 0.25f * k) * (1.0f - 0.3f * calm);
        const float xScale = 1.0f;   // never stretch: circles stay circles in the wide strip

        for (int i = 0; i < n; ++i)
        {
            float v = disp[(size_t) i];
            if (i < blend)   // fold the end of the window into the start so the loop closes seamlessly
            {
                const float w = (float) i / (float) blend;
                v = disp[(size_t) (n + i)] * (1.0f - w) + v * w;
            }

            const float th = rotation + twoPi * (float) i / (float) n;
            const float deform = 1.0f + 0.07f * weight * k * std::sin (2.0f * th + time * 0.4f);
            const float r = radius * deform + v * orbitAmp;
            const juce::Point<float> p (centre.x + std::sin (th) * r * xScale, centre.y - std::cos (th) * r);

            if (i == 0) current.startNewSubPath (p); else current.lineTo (p);

            if (detail > 0.03f)
            {
                const float rd = radius + orbitAmp * 0.95f + high[(size_t) i] * orbitAmp * 2.0f * detail;
                const juce::Point<float> q (centre.x + std::sin (th) * rd * xScale, centre.y - std::cos (th) * rd);
                if (i == 0) detailPath.startNewSubPath (q); else detailPath.lineTo (q);
            }
        }
        current.closeSubPath();
        if (! detailPath.isEmpty())
            detailPath.closeSubPath();

        pushTrail (current);
    }
    else // Mirror
    {
        const int n = 420;
        const int dec = juce::jmax (1, juce::roundToInt (sr / 16000.0));
        const int d = juce::jmax (2, juce::roundToInt (sr * 0.0011 / (double) dec));
        extractTriggered (h, gain, f.peak, sr, (n + d) * dec, raw, n + d);
        settle();

        const float amp = juce::jmin (a.getWidth(), a.getHeight()) * (strip ? 0.46f : 0.40f) * (1.0f + 0.08f * pulse);
        mirrorAmp = amp;
        const float xScale = 1.0f;   // never stretch: circles stay circles in the wide strip
        constexpr float r2 = 0.70710678f;

        // phase-space portrait, rotated 45 degrees (centred on the origin; placed when drawing)
        for (int i = 0; i < n; ++i)
        {
            const float x = disp[(size_t) i], y = disp[(size_t) (i + d)];
            const juce::Point<float> p ((x - y) * r2 * amp * xScale, -(x + y) * r2 * amp);
            if (i == 0) current.startNewSubPath (p); else current.lineTo (p);
        }

        pushTrail (current);
    }
}

juce::Path Visualiser::revealRegion (float margin) const
{
    juce::Path p;

    if (mode == VisualMode::Classic && env.size() >= 2)
    {
        const int cols = (int) env.size();
        const float cy = area.getCentreY();
        const float dx = area.getWidth() / (float) (cols - 1);
        for (int c = 0; c < cols; ++c)
        {
            const float y = cy - env[(size_t) c] * halfHeight - margin;
            if (c == 0) p.startNewSubPath (area.getX(), y); else p.lineTo (area.getX() + (float) c * dx, y);
        }
        for (int c = cols - 1; c >= 0; --c)
            p.lineTo (area.getX() + (float) c * dx, cy + env[(size_t) c] * halfHeight + margin);
        p.closeSubPath();
        return p.createPathWithRoundedCorners (margin);
    }

    if (mode == VisualMode::Orbit)
    {
        const float outer = orbitR + orbitAmp + margin, inner = juce::jmax (0.0f, orbitR - orbitAmp - margin);
        p.addEllipse (centre.x - outer, centre.y - outer, outer * 2.0f, outer * 2.0f);
        p.addEllipse (centre.x - inner, centre.y - inner, inner * 2.0f, inner * 2.0f);
        p.setUsingNonZeroWinding (false);
        return p;
    }

    if (mode == VisualMode::Mirror)
    {
        const float r = mirrorAmp * 1.05f + margin;
        p.addEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);
        return p;
    }

    return p;   // NONE: no mask
}

// =================================================================================================
//  Drawing
// =================================================================================================
void Visualiser::strokeLuminous (juce::Graphics& g, const juce::Path& p, juce::Colour core, juce::Colour halo,
                                 float width, float glowAmount, bool lightTheme) const
{
    const float gs = lightTheme ? 0.5f : 1.0f;
    glowAmount *= 1.0f + 0.9f * pulseBoost;
    width *= 1.0f + 0.3f * pulseBoost;
    g.setColour (halo.withAlpha (juce::jlimit (0.0f, 1.0f, 0.05f * glowAmount * gs)));
    g.strokePath (p, stroke (width * 8.0f));
    g.setColour (halo.withAlpha (juce::jlimit (0.0f, 1.0f, 0.14f * glowAmount * gs)));
    g.strokePath (p, stroke (width * 3.4f));
    g.setColour (core.withAlpha (0.95f));
    g.strokePath (p, stroke (width));
}

void Visualiser::draw (juce::Graphics& g, const look::Look& lk, const look::Palette& pal, float beatPulse) const
{
    // BEAT COLOUR: the line's glow swells on each beat
    pulseBoost = beatPulse;

    switch (mode)
    {
        case VisualMode::Classic: drawClassic (g, lk, pal, beatPulse); break;
        case VisualMode::Orbit:   drawOrbit (g, lk, pal, beatPulse);   break;
        case VisualMode::Mirror:  drawMirror (g, lk, pal, beatPulse);  break;
        case VisualMode::None:    break;
    }
}

void Visualiser::drawClassic (juce::Graphics& g, const look::Look& lk, const look::Palette& pal, float beatPulse) const
{
    if (current.isEmpty())
        return;

    const bool light = lk.light;
    const float w = (strip ? 1.15f : 1.45f) * (0.75f + 1.2f * weight * k) * (1.0f - 0.35f * calm);
    const auto core = light ? pal.a : pal.a.interpolatedWith (juce::Colours::white, 0.12f + 0.2f * pulse);

    // layered trails - longer and softer for sustained, atmospheric material
    const int numTrails = juce::jmin (trailCount - 1, 2 + (int) (flow * 5.0f));
    for (int i = numTrails; i >= 1; --i)
    {
        const auto& p = trails[(size_t) ((trailHead - 1 - i + maxTrails * 2) % maxTrails)];
        g.setColour (pal.b.withAlpha (juce::jlimit (0.0f, 1.0f, 0.18f * std::pow (0.68f, (float) i) * (0.5f + flow))));
        g.strokePath (p, stroke (w * 0.6f));
    }

    // kick: a soft bloom around the line
    const float bloom = juce::jmax (pulse, beatPulse * 0.5f);
    if (bloom > 0.02f)
    {
        g.setColour (pal.b.withAlpha (0.06f * bloom * (light ? 0.5f : 1.0f)));
        g.strokePath (current, stroke (w * 12.0f));
    }

    // drops: the line separates into several delicate luminous lines, then rejoins
    if (split > 0.02f)
    {
        for (int j = -2; j <= 2; ++j)
        {
            const int aj = std::abs (j);
            if (j == 0 || (aj == 2 && split < 0.45f))
                continue;

            const float off = (float) j * split * halfHeight * 0.14f * (0.75f + 0.25f * std::sin (time * 0.8f + (float) j * 1.7f));
            const float sc = 1.0f - 0.1f * (float) aj * split;
            auto p = current;
            p.applyTransform (juce::AffineTransform::scale (1.0f, sc, 0.0f, centre.y).translated (0.0f, off));
            g.setColour (pal.b.interpolatedWith (pal.c, 0.3f * (float) aj).withAlpha ((0.6f - 0.15f * (float) aj) * split));
            g.strokePath (p, stroke (w * 0.55f));
        }
    }

    strokeLuminous (g, current, core, pal.b, w, glow, light);

    // hi-hats and fine percussion: a delicate, fast-moving detail line
    if (detail > 0.03f && ! detailPath.isEmpty())
    {
        g.setColour (pal.c.withAlpha (juce::jlimit (0.0f, 1.0f, 0.5f * detail)));
        g.strokePath (detailPath, stroke (0.7f));
    }
}

void Visualiser::drawOrbit (juce::Graphics& g, const look::Look& lk, const look::Palette& pal, float beatPulse) const
{
    if (current.isEmpty())
        return;

    const bool light = lk.light;
    const float w = (strip ? 1.1f : 1.35f) * (0.75f + 1.0f * weight * k) * (1.0f - 0.3f * calm);
    const auto core = light ? pal.a : pal.a.interpolatedWith (juce::Colours::white, 0.12f + 0.2f * pulse);
    const float xScale = 1.0f;   // never stretch: circles stay circles in the wide strip

    // breathing core light
    {
        const float a = (0.05f + 0.10f * weight + 0.08f * juce::jmax (pulse, beatPulse)) * (light ? 0.6f : 1.0f);
        juce::ColourGradient cg (pal.b.withAlpha (juce::jlimit (0.0f, 1.0f, a)), centre.x, centre.y,
                                 pal.b.withAlpha (0.0f), centre.x + orbitR * 1.2f, centre.y, true);
        juce::FillType ft (cg);
        ft.transform = juce::AffineTransform::scale (xScale, 1.0f, centre.x, centre.y);
        g.setFillType (ft);
        g.fillEllipse (centre.x - orbitR * 1.2f * xScale, centre.y - orbitR * 1.2f, orbitR * 2.4f * xScale, orbitR * 2.4f);
    }

    // hairline guide
    g.setColour ((light ? juce::Colours::black : juce::Colours::white).withAlpha (0.06f));
    g.drawEllipse (centre.x - orbitR * xScale, centre.y - orbitR, orbitR * 2.0f * xScale, orbitR * 2.0f, 0.8f);

    // echoes expanding outwards
    const int numTrails = juce::jmin (trailCount - 1, 3 + (int) (flow * 4.0f));
    for (int i = numTrails; i >= 1; --i)
    {
        auto p = trails[(size_t) ((trailHead - 1 - i + maxTrails * 2) % maxTrails)];
        const float s = 1.0f + 0.035f * (float) i;
        p.applyTransform (juce::AffineTransform::scale (s, s, centre.x, centre.y));
        g.setColour (pal.b.withAlpha (juce::jlimit (0.0f, 1.0f, 0.2f * std::pow (0.7f, (float) i) * (0.4f + flow + 0.3f * split))));
        g.strokePath (p, stroke (w * 0.6f));
    }

    // energetic passages: rotational symmetry appears
    if (split > 0.02f)
    {
        for (int j = 1; j <= 2; ++j)
        {
            auto p = current;
            p.applyTransform (juce::AffineTransform::rotation ((float) j * twoPi / 3.0f, centre.x, centre.y));
            g.setColour (pal.c.interpolatedWith (pal.b, 0.5f).withAlpha (0.35f * split));
            g.strokePath (p, stroke (w * 0.5f));
        }
    }

    strokeLuminous (g, current, core, pal.b, w, glow, light);

    if (detail > 0.03f && ! detailPath.isEmpty())
    {
        g.setColour (pal.c.withAlpha (juce::jlimit (0.0f, 1.0f, 0.45f * detail)));
        g.strokePath (detailPath, stroke (0.7f));
    }
}

void Visualiser::drawMirror (juce::Graphics& g, const look::Look& lk, const look::Palette& pal, float beatPulse) const
{
    if (current.isEmpty())
        return;

    const bool light = lk.light;
    const float w = (strip ? 1.0f : 1.2f) * (0.75f + 1.0f * weight * k) * (1.0f - 0.3f * calm);
    const auto core = light ? pal.a : pal.a.interpolatedWith (juce::Colours::white, 0.1f + 0.2f * pulse);
    const float spin = rotation * 0.25f;

    auto placed = [&] (const juce::Path& src, float angle, float mirror)
    {
        auto p = src;
        p.applyTransform (juce::AffineTransform::scale (mirror, 1.0f).rotated (angle + spin).translated (centre.x, centre.y));
        return p;
    };

    // soft centre light
    {
        const float r = juce::jmin (area.getWidth(), area.getHeight()) * 0.45f;
        const float a = (0.04f + 0.08f * weight + 0.06f * juce::jmax (pulse, beatPulse)) * (light ? 0.6f : 1.0f);
        juce::ColourGradient cg (pal.b.withAlpha (juce::jlimit (0.0f, 1.0f, a)), centre.x, centre.y,
                                 pal.b.withAlpha (0.0f), centre.x + r, centre.y, true);
        g.setGradientFill (cg);
        g.fillEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);
    }

    const float baseAngles[] = { 0.0f, pi };
    const float mirrors[] = { 1.0f, -1.0f };

    // trails of the four base copies
    const int numTrails = juce::jmin (trailCount - 1, 2 + (int) (flow * 3.0f));
    for (int i = numTrails; i >= 1; --i)
    {
        const auto& t = trails[(size_t) ((trailHead - 1 - i + maxTrails * 2) % maxTrails)];
        g.setColour (pal.b.withAlpha (juce::jlimit (0.0f, 1.0f, 0.12f * std::pow (0.65f, (float) i) * (0.5f + flow))));
        for (float ang : baseAngles)
            for (float m : mirrors)
                g.strokePath (placed (t, ang, m), stroke (w * 0.5f));
    }

    // higher symmetry grows with the music's energy (6-fold kaleidoscope at full strength)
    if (symmetry > 0.02f)
    {
        const float extra[] = { pi / 3.0f, 2.0f * pi / 3.0f, 4.0f * pi / 3.0f, 5.0f * pi / 3.0f };
        g.setColour (pal.c.interpolatedWith (pal.b, 0.4f).withAlpha (juce::jlimit (0.0f, 1.0f, 0.5f * symmetry)));
        for (float ang : extra)
            for (float m : mirrors)
                g.strokePath (placed (current, ang, m), stroke (w * 0.55f));
    }

    for (float ang : baseAngles)
        for (float m : mirrors)
            strokeLuminous (g, placed (current, ang, m), m > 0.0f ? core : core.interpolatedWith (pal.b, 0.35f), pal.b, w,
                            glow * (m > 0.0f ? 1.0f : 0.7f), light);
}

void Visualiser::drawSpectrum (juce::Graphics& g, const AudioFeatures& f, juce::Rectangle<float> r,
                               const look::Look& lk, const look::Palette& pal, bool prominent) const
{
    const int n = AudioFeatures::numSpectrumBands;
    if (r.getWidth() < 8.0f || r.getHeight() < 4.0f)
        return;

    const float dx = r.getWidth() / (float) (n - 1);
    const float hScale = prominent ? 0.92f : 1.0f;
    juce::Path line;

    for (int i = 0; i < n; ++i)
    {
        const float x = r.getX() + dx * (float) i;
        const float y = r.getBottom() - f.spectrum[(size_t) i] * r.getHeight() * hScale;
        if (i == 0) line.startNewSubPath (x, y); else line.lineTo (x, y);
    }
    line = line.createPathWithRoundedCorners (dx * 0.7f);

    auto fill = line;
    fill.lineTo (r.getRight(), r.getBottom());
    fill.lineTo (r.getX(), r.getBottom());
    fill.closeSubPath();

    const float a = prominent ? 1.0f : 0.45f;

    if (prominent)
    {
        g.setColour (lk.grid);
        for (int i = 0; i < n; i += 2)
            g.fillRect (r.getX() + dx * (float) i, r.getY(), 1.0f, r.getHeight());
    }

    juce::ColourGradient grad (pal.b.withAlpha (0.28f * a), 0.0f, r.getY(), pal.b.withAlpha (0.0f), 0.0f, r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillPath (fill);

    if (prominent)
    {
        auto reflection = fill;
        reflection.applyTransform (juce::AffineTransform::scale (1.0f, -0.3f, 0.0f, r.getBottom()));
        g.setColour (pal.b.withAlpha (0.1f));
        g.fillPath (reflection);
    }

    g.setColour (pal.a.withAlpha (prominent ? 0.9f : 0.35f));
    g.strokePath (line, juce::PathStrokeType (prominent ? 1.4f : 1.0f));

    if (prominent)
    {
        g.setColour (pal.c.withAlpha (0.85f));
        for (int i = 0; i < n; ++i)
        {
            const float y = r.getBottom() - peaks[(size_t) i] * r.getHeight() * hScale;
            g.fillRect (r.getX() + dx * (float) i - dx * 0.25f, y - 1.0f, dx * 0.5f, 1.5f);
        }
    }
}
}

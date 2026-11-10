#include "Oscilloscope.h"
#include "Palette.h"
#include "Util.h"
#include <cmath>

namespace token
{
namespace
{
    void glowStroke (juce::Graphics& g, const juce::Path& p, juce::Colour c, float intensity, float scale, float kick)
    {
        static constexpr float widths[] = { 18.0f, 10.0f, 5.5f, 2.8f };
        static constexpr float alphas[] = { 0.028f, 0.06f, 0.13f, 0.30f };

        for (int i = 0; i < 4; ++i)
        {
            g.setColour (c.withAlpha (juce::jlimit (0.0f, 1.0f, alphas[i] * intensity)));
            g.strokePath (p, juce::PathStrokeType (widths[i] * scale * (1.0f + 0.25f * kick),
                                                   juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        g.setColour (c.interpolatedWith (pal::core, juce::jlimit (0.0f, 1.0f, 0.5f + 0.3f * kick)).withAlpha (0.95f));
        g.strokePath (p, juce::PathStrokeType ((1.1f + 0.7f * kick) * juce::jmax (0.8f, scale),
                                               juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void Oscilloscope::resizeBuffers (int newCols)
{
    cols = newCols;
    for (auto* v : { &rawA, &rawB, &minV, &maxV, &smoothWave, &smoothEnv })
        v->assign ((size_t) cols, 0.0f);
    trailCount = 0;
}

Oscilloscope::Lane Oscilloscope::laneGeometry (int lane) const noexcept
{
    if (stereo && ! scrolling)
    {
        const float h = area.getHeight();
        return { area.getY() + h * (lane == 0 ? 0.27f : 0.73f), h * 0.21f };
    }
    return { area.getCentreY(), area.getHeight() * 0.46f };
}

void Oscilloscope::update (const ScopeHistory& h, const AudioFeatures& f, const ScopeSettings& s,
                           juce::Rectangle<float> a, double sampleRate, float dt)
{
    area = a;
    const int newCols = juce::jmax (32, (int) (a.getWidth() / 2.0f));

    if (newCols != cols || s.scrolling != lastScrolling || s.stereo != lastStereo)
    {
        resizeBuffers (newCols);
        lastScrolling = s.scrolling;
        lastStereo = s.stereo;
    }

    scrolling = s.scrolling;
    stereo = s.stereo;

    const double sr = juce::jmax (8000.0, sampleRate);
    const float gain = s.autoGain ? f.scopeGain : 1.0f;
    const auto total = h.getTotal();
    const auto oldest = h.oldest();
    auto shape = [gain] (float v) { return std::tanh (v * gain); };   // gentle soft-clip keeps peaks on screen

    if (scrolling)
    {
        const int window = (int) juce::jmin ((double) (ScopeHistory::size - 8192), sr * (s.strip ? 1.6 : 1.0));
        const int spc = juce::jmax (1, window / cols);
        const juce::int64 end = (total / spc) * spc;     // bucket-aligned: stable scrolling

        for (int c = 0; c < cols; ++c)
        {
            const juce::int64 b0 = end - (juce::int64) (cols - c) * spc;
            float hi = 0.0f, lo = 0.0f;

            for (int i = 0; i < spc; ++i)
            {
                const auto idx = b0 + i;
                if (idx < oldest || idx >= total)
                    continue;

                if (stereo)
                {
                    hi = juce::jmax (hi,  std::abs (h.left (idx)));
                    lo = juce::jmin (lo, -std::abs (h.right (idx)));
                }
                else
                {
                    const float m = h.mono (idx);
                    hi = juce::jmax (hi, m);
                    lo = juce::jmin (lo, m);
                }
            }

            maxV[(size_t) c] = shape (hi);
            minV[(size_t) c] = shape (lo);
            rawA[(size_t) c] = maxV[(size_t) c];
        }
    }
    else
    {
        const int displayLen = (int) juce::jlimit (256.0, 8192.0, sr * (s.strip ? 0.055 : 0.045));
        const juce::int64 latestStart = total - displayLen - 1;
        const juce::int64 searchFrom = juce::jmax (oldest + 2, latestStart - displayLen);
        double startPos = (double) latestStart;

        if (latestStart > searchFrom + 8)
        {
            const int n = (int) (latestStart - searchFrom + 1);
            lpScratch.resize ((size_t) n);
            negScratch.resize ((size_t) n);

            const float coeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 250.0f / (float) sr);
            const float threshold = 0.08f * juce::jmax (1.0e-4f, f.peak);
            float lp = h.mono (searchFrom - 1);
            int lastNeg = -1;

            for (int k = 0; k < n; ++k)
            {
                lp += (h.mono (searchFrom + k) - lp) * coeff;
                lpScratch[(size_t) k] = lp;
                if (lp < -threshold)
                    lastNeg = k;
                negScratch[(size_t) k] = lastNeg;
            }

            // most recent armed rising zero-crossing
            for (int k = n - 1; k >= 1; --k)
            {
                const float p0 = lpScratch[(size_t) (k - 1)], p1 = lpScratch[(size_t) k];
                const int neg = negScratch[(size_t) (k - 1)];

                if (p0 < 0.0f && p1 >= 0.0f && neg >= 0 && (k - neg) < displayLen / 2)
                {
                    const float frac = p0 / (p0 - p1);
                    startPos = (double) (searchFrom + k - 1) + (double) frac;
                    break;
                }
            }
        }

        auto lerpAt = [&h, oldest, total] (double pos, int channel)
        {
            const auto i0 = (juce::int64) std::floor (pos);
            const float fr = (float) (pos - (double) i0);

            auto get = [&] (juce::int64 i) -> float
            {
                if (i < oldest || i >= total) return 0.0f;
                return channel == 0 ? h.left (i) : (channel == 1 ? h.right (i) : h.mono (i));
            };

            const float v0 = get (i0);
            return v0 + (get (i0 + 1) - v0) * fr;
        };

        const double step = (double) displayLen / (double) (cols - 1);

        for (int c = 0; c < cols; ++c)
        {
            const double pos = startPos + (double) c * step;

            if (stereo)
            {
                rawA[(size_t) c] = shape (lerpAt (pos, 0));
                rawB[(size_t) c] = shape (lerpAt (pos, 1));
            }
            else
            {
                rawA[(size_t) c] = shape (lerpAt (pos, 2));
            }
        }
    }

    // ---- smoothed copies for the living world
    const float a1 = 1.0f - std::exp (-dt / 0.10f);
    for (int c = 0; c < cols; ++c)
        smoothWave[(size_t) c] += (rawA[(size_t) c] - smoothWave[(size_t) c]) * a1;

    for (int c = 0; c < cols; ++c)
    {
        float m = 0.0f;
        for (int k = juce::jmax (0, c - 4); k <= juce::jmin (cols - 1, c + 4); ++k)
            m = juce::jmax (m, scrolling ? maxV[(size_t) k] : std::abs (rawA[(size_t) k]));

        smoothEnv[(size_t) c] = smoothTo (smoothEnv[(size_t) c], m, dt, 0.04f, 0.45f);
    }

    // ---- paths
    const auto laneA = laneGeometry (0);
    const auto laneB = laneGeometry (1);
    lineA.clear();
    lineB.clear();
    envelopeFill.clear();

    if (scrolling)
    {
        buildLinePath (lineA, maxV, laneA.centre, laneA.half);
        buildLinePath (lineB, minV, laneA.centre, laneA.half);

        envelopeFill = lineA;
        const float dx = area.getWidth() / (float) (cols - 1);
        for (int c = cols - 1; c >= 0; --c)
            envelopeFill.lineTo (area.getX() + (float) c * dx, laneA.centre - minV[(size_t) c] * laneA.half);
        envelopeFill.closeSubPath();
    }
    else
    {
        buildLinePath (lineA, rawA, laneA.centre, laneA.half);
        if (stereo)
            buildLinePath (lineB, rawB, laneB.centre, laneB.half);
    }

    auto& slot = trails[(size_t) trailHead];
    slot = lineA;
    if (stereo && ! scrolling)
        slot.addPath (lineB);

    trailHead = (trailHead + 1) % maxTrails;
    trailCount = juce::jmin (trailCount + 1, maxTrails);
}

void Oscilloscope::buildLinePath (juce::Path& p, const std::vector<float>& v, float centre, float halfHeight) const
{
    const float x0 = area.getX();
    const float dx = area.getWidth() / (float) (cols - 1);
    auto pt = [&] (int c) { return juce::Point<float> (x0 + (float) c * dx, centre - v[(size_t) c] * halfHeight); };

    p.preallocateSpace (cols * 5);
    p.startNewSubPath (pt (0));

    for (int c = 1; c < cols - 1; ++c)
    {
        const auto p1 = pt (c);
        const auto p2 = pt (c + 1);
        p.quadraticTo (p1, (p1 + p2) * 0.5f);
    }

    p.lineTo (pt (cols - 1));
}

float Oscilloscope::sampleColumn (const std::vector<float>& v, float x) const noexcept
{
    if (cols < 2 || area.getWidth() <= 0.0f || (int) v.size() < cols)
        return 0.0f;

    const float pos = juce::jlimit (0.0f, (float) (cols - 1), (x - area.getX()) / area.getWidth() * (float) (cols - 1));
    const int i0 = (int) pos;
    const int i1 = juce::jmin (cols - 1, i0 + 1);
    const float fr = pos - (float) i0;
    return v[(size_t) i0] + (v[(size_t) i1] - v[(size_t) i0]) * fr;
}

juce::Path Oscilloscope::signalBand (float margin) const
{
    juce::Path band;
    if (cols < 2 || (int) smoothEnv.size() < cols)
        return band;

    const auto lane = laneGeometry (0);
    const float dx = area.getWidth() / (float) (cols - 1);
    const int step = juce::jmax (1, cols / 120);

    for (int c = 0; c < cols; c += step)
    {
        const float y = lane.centre - smoothEnv[(size_t) c] * lane.half - margin;
        if (c == 0) band.startNewSubPath (area.getX(), y);
        else        band.lineTo (area.getX() + (float) c * dx, y);
    }
    band.lineTo (area.getRight(), lane.centre - smoothEnv[(size_t) (cols - 1)] * lane.half - margin);

    for (int c = cols - 1; c >= 0; c -= step)
        band.lineTo (area.getX() + (float) c * dx, lane.centre + smoothEnv[(size_t) c] * lane.half + margin);
    band.lineTo (area.getX(), lane.centre + smoothEnv[0] * lane.half + margin);
    band.closeSubPath();

    return band.createPathWithRoundedCorners (margin * 0.8f);
}

float Oscilloscope::waveYAt (float x) const noexcept
{
    const auto lane = laneGeometry (0);
    return lane.centre - sampleColumn (smoothWave, x) * lane.half;
}

float Oscilloscope::envelopeTopAt (float x) const noexcept
{
    const auto lane = laneGeometry (0);
    return lane.centre - sampleColumn (smoothEnv, x) * lane.half;
}

juce::Colour Oscilloscope::lineColour (const AudioFeatures& f) const
{
    const auto dream = pal::turquoise.interpolatedWith (pal::moon, 0.25f);
    const auto pulse = pal::cyan;
    const auto chaos = pal::cyan.interpolatedWith (pal::core, 0.12f);
    return pal::mix3 (f.stateWeights, dream, pulse, chaos);
}

void Oscilloscope::draw (juce::Graphics& g, const AudioFeatures& f, const ScopeSettings& s) const
{
    if (cols < 2)
        return;

    const auto base = lineColour (f);
    const float glow = s.glow;
    const float kick = juce::jlimit (0.0f, 1.0f, f.kick * (0.3f + 0.9f * s.react));
    const float scale = juce::jlimit (0.55f, 1.25f, area.getHeight() / 320.0f);

    // ---- phosphor persistence: previous frames, fading
    const int wanted = scrolling ? 2 : 2 + (int) (glow * 9.0f);
    const int numTrails = juce::jmin (wanted, trailCount - 1);

    for (int i = numTrails; i >= 1; --i)
    {
        const auto& p = trails[(size_t) ((trailHead - 1 - i + maxTrails * 2) % maxTrails)];
        const float alpha = 0.30f * std::pow (0.74f, (float) i) * (0.35f + glow);
        g.setColour (base.withAlpha (juce::jlimit (0.0f, 1.0f, alpha)));
        g.strokePath (p, juce::PathStrokeType (1.2f * scale + 0.6f));
    }

    const float intensity = (0.35f + 1.25f * glow) * (1.0f + kick) * (0.9f + 0.35f * f.stateWeights[2]);

    if (scrolling)
    {
        juce::ColourGradient fill (base.withAlpha (0.04f), 0.0f, area.getY(), base.withAlpha (0.04f), 0.0f, area.getBottom(), false);
        fill.addColour (0.5, base.withAlpha (juce::jlimit (0.0f, 1.0f, 0.2f + 0.2f * glow)));
        g.setGradientFill (fill);
        g.fillPath (envelopeFill);

        glowStroke (g, lineA, base, intensity * 0.8f, scale * 0.8f, kick);
        glowStroke (g, lineB, stereo ? base.interpolatedWith (pal::gold, 0.5f) : base, intensity * 0.8f, scale * 0.8f, kick);
    }
    else
    {
        // CHAOS: a warm golden corona around the trace
        const float chaosW = f.stateWeights[2];
        if (chaosW > 0.02f)
        {
            g.setColour (pal::gold.withAlpha (juce::jlimit (0.0f, 1.0f, 0.05f * chaosW * (0.5f + glow))));
            g.strokePath (lineA, juce::PathStrokeType (30.0f * scale, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        glowStroke (g, lineA, base, intensity, scale, kick);
        if (stereo)
            glowStroke (g, lineB, base.interpolatedWith (pal::gold, 0.55f), intensity, scale, kick);
    }
}

void Oscilloscope::drawSpectrum (juce::Graphics& g, const AudioFeatures& f, juce::Rectangle<float> r, float glow) const
{
    const int n = AudioFeatures::numSpectrumBands;
    const float dx = r.getWidth() / (float) (n - 1);
    juce::Path line;

    for (int i = 0; i < n; ++i)
    {
        const float x = r.getX() + dx * (float) i;
        const float y = r.getBottom() - f.spectrum[(size_t) i] * r.getHeight();
        if (i == 0) line.startNewSubPath (x, y);
        else        line.lineTo (x, y);
    }

    line = line.createPathWithRoundedCorners (dx * 0.6f);

    auto fill = line;
    fill.lineTo (r.getRight(), r.getBottom());
    fill.lineTo (r.getX(), r.getBottom());
    fill.closeSubPath();

    juce::ColourGradient grad (pal::gold.withAlpha (0.16f * (0.4f + glow)), 0.0f, r.getY(),
                               pal::gold.withAlpha (0.0f), 0.0f, r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillPath (fill);

    g.setColour (pal::amber.withAlpha (juce::jlimit (0.0f, 1.0f, 0.3f * (0.4f + glow))));
    g.strokePath (line, juce::PathStrokeType (1.0f));

    for (int i = 0; i < n; ++i)
    {
        const float v = f.spectrum[(size_t) i];
        const float x = r.getX() + dx * (float) i;
        const float y = r.getBottom() - v * r.getHeight();
        g.setColour (pal::goldBright.withAlpha (juce::jlimit (0.0f, 1.0f, 0.15f + 0.5f * v)));
        g.fillRect (x - 0.75f, y - 0.75f, 1.5f, 1.5f);
    }
}
}

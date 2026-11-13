#include "ScopePen.h"

namespace token::engine
{
namespace
{
    /** Triangle fold so the waveform is continuous around closed shapes. */
    inline float tri (float x)
    {
        const float f = x - std::floor (x);
        return 1.0f - std::abs (2.0f * f - 1.0f);
    }
}

ScopePen::ScopePen()
{
    scr.reserve (8192);
    strokeLen.reserve (512);
}

void ScopePen::build (const Sketch& sk, const juce::AffineTransform& xf, const PenStyle& st, const SignalFrame& sf, juce::Path& out)
{
    out.clear();
    const int ns = sk.strokes();
    if (ns == 0)
        return;

    // 1. map to the screen and measure
    scr.resize (sk.pts.size());
    for (size_t i = 0; i < sk.pts.size(); ++i)
        scr[i] = sk.pts[i].transformedBy (xf);

    strokeLen.resize ((size_t) ns);
    float total = 0.0f;
    for (int s = 0; s < ns; ++s)
    {
        const int a = sk.starts[(size_t) s], b = sk.strokeEnd (s);
        float len = 0.0f;
        for (int i = a + 1; i < b; ++i)
            len += scr[(size_t) i - 1].getDistanceFrom (scr[(size_t) i]);
        if (sk.closedFlags[(size_t) s] && b - a > 2)
            len += scr[(size_t) b - 1].getDistanceFrom (scr[(size_t) a]);
        strokeLen[(size_t) s] = len;
        total += len;
    }
    if (total < 1.0f)
        return;

    const float step = juce::jmax (st.stepPx, total / (float) juce::jmax (64, st.maxPoints));
    const auto& wave = st.useFine ? sf.fine : sf.wave;
    const float fineGain = st.useFine ? 2.5f : 1.0f;
    const float reveal = juce::jlimit (0.0f, 1.0f, st.reveal);

    // 2. walk each stroke at even spacing, pushing every sample sideways by the waveform
    float before = 0.0f;
    for (int s = 0; s < ns; ++s)
    {
        const int a = sk.starts[(size_t) s], b = sk.strokeEnd (s);
        const bool closed = sk.closedFlags[(size_t) s] && b - a > 2;
        const int segCount = (b - a - 1) + (closed ? 1 : 0);
        const float len = strokeLen[(size_t) s];
        const float drawLen = len * reveal;

        if (segCount <= 0 || drawLen < 0.5f)
        {
            before += len;
            continue;
        }

        const int samples = juce::jmax (2, (int) (drawLen / step) + 1);
        int seg = 0;
        float segStart = 0.0f;

        auto segPoint = [&] (int k, bool end) -> juce::Point<float>
        {
            const int i0 = a + k;
            const int i1 = (k + 1 < b - a) ? a + k + 1 : a;   // closing segment wraps to the start
            return scr[(size_t) (end ? i1 : i0)];
        };

        for (int j = 0; j < samples; ++j)
        {
            const float d = drawLen * (float) j / (float) (samples - 1);

            // advance to the segment containing distance d
            float segLen = segPoint (seg, false).getDistanceFrom (segPoint (seg, true));
            while (seg < segCount - 1 && segStart + segLen < d)
            {
                segStart += segLen;
                ++seg;
                segLen = segPoint (seg, false).getDistanceFrom (segPoint (seg, true));
            }

            const auto p0 = segPoint (seg, false), p1 = segPoint (seg, true);
            const float t = segLen > 1.0e-4f ? juce::jlimit (0.0f, 1.0f, (d - segStart) / segLen) : 0.0f;
            const auto p = p0 + (p1 - p0) * t;
            auto dir = p1 - p0;
            const float dl = std::sqrt (dir.x * dir.x + dir.y * dir.y);
            dir = dl > 1.0e-4f ? dir / dl : juce::Point<float> (1.0f, 0.0f);

            const float u = (before + d) / total;
            const float v = fx::sampleAt (wave, tri (u * st.cycles + st.phase)) * fineGain;
            const juce::Point<float> q (p.x - dir.y * v * st.amp, p.y + dir.x * v * st.amp);

            if (j == 0) out.startNewSubPath (q); else out.lineTo (q);
        }

        if (closed && reveal >= 0.999f)
            out.closeSubPath();

        before += len;
    }
}

void ScopePen::glow (juce::Graphics& g, const juce::Path& p, juce::Colour core, juce::Colour halo, float width,
                     float amount, int quality, const juce::AffineTransform& xf)
{
    if (core.getFloatAlpha() < 0.004f)
        return;

    if (quality >= 1 && amount > 0.02f)
    {
        g.setColour (halo.withAlpha (juce::jlimit (0.0f, 1.0f, 0.09f * amount * halo.getFloatAlpha())));
        g.strokePath (p, fx::pen (width * 4.5f), xf);
    }
    if (quality >= 2 && amount > 0.25f)
    {
        g.setColour (halo.withAlpha (juce::jlimit (0.0f, 1.0f, 0.18f * amount * halo.getFloatAlpha())));
        g.strokePath (p, fx::pen (width * 2.2f), xf);
    }
    g.setColour (core);
    g.strokePath (p, fx::pen (width), xf);
}
}

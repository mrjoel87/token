#include "SignalWorld.h"
#include "../Util.h"

namespace token::engine
{
void SignalWorld::update (const SignalFrame& s)
{
    split = juce::jmax (split * std::exp (-s.dt / 0.12f), s.snare);
    zoom = smoothTo (zoom, s.kick, s.dt, 0.01f, 0.25f);

    if (s.locked)
        return;   // LOCK freezes the ribbon history; the live trace keeps moving

    // capture the waveform into the ribbon history at a rate set by SPEED
    pushClock += s.dt * s.speed;
    if (pushClock >= 1.0f / 24.0f)
    {
        pushClock = 0.0f;
        auto& slot = history[(size_t) head];
        for (int i = 0; i < points; ++i)
            slot[(size_t) i] = fx::sampleAt (s.slow, (float) i / (float) (points - 1));
        head = (head + 1) % layers;
        filled = juce::jmin (filled + 1, layers);
    }
}

void SignalWorld::draw (juce::Graphics& g, const SignalFrame& s, float alpha) const
{
    if (alpha <= 0.005f)
        return;

    const auto A = s.area;
    const auto& pal = s.pal;
    const int q = s.quality;
    const bool strip = A.getWidth() / juce::jmax (1.0f, A.getHeight()) > 1.9f;
    const float react = 0.35f + 0.9f * s.react;
    const float cy = A.getCentreY();
    const float half = A.getHeight() * (strip ? 0.34f : 0.26f) * (1.0f + 0.12f * s.kick);

    // hairline graticule
    g.setColour (pal.b.withAlpha (0.07f * alpha));
    g.fillRect (A.getX(), cy, A.getWidth(), 1.0f);
    for (int i = 1; i < 8; ++i)
        g.fillRect (A.getX() + A.getWidth() * (float) i / 8.0f, A.getY(), 1.0f, A.getHeight());

    // spectrum floor, mirrored
    {
        const auto* f = s.audio;
        if (f != nullptr)
        {
            const int n = AudioFeatures::numSpectrumBands;
            const float base = A.getBottom() - A.getHeight() * 0.02f;
            const float h = A.getHeight() * (strip ? 0.3f : 0.18f);
            spectrum.clear();
            for (int i = 0; i < n; ++i)
            {
                const float x = A.getX() + A.getWidth() * (float) i / (float) (n - 1);
                const float y = base - f->spectrum[(size_t) i] * h;
                if (i == 0) spectrum.startNewSubPath (x, y); else spectrum.lineTo (x, y);
            }
            g.setColour (pal.b.withAlpha (0.35f * alpha));
            g.strokePath (spectrum, fx::pen (1.0f));
            if (q >= 1)
            {
                for (int i = 0; i < n; i += 2)
                {
                    const float x = A.getX() + A.getWidth() * (float) i / (float) (n - 1);
                    const float y = base - f->spectrum[(size_t) i] * h;
                    g.setColour (pal.a.withAlpha ((0.15f + 0.6f * s.high) * alpha));
                    g.fillRect (x - 0.75f, y - 0.75f, 1.5f, 1.5f);
                }
            }
        }
    }

    // ribbons / tunnel from the recent history (oldest first)
    const int count = juce::jmin (filled, q >= 2 ? layers : (q == 1 ? layers / 2 : layers / 4));
    const juce::Point<float> vp (A.getCentreX(), cy - A.getHeight() * 0.08f);
    for (int n = count; n >= 1; --n)
    {
        const auto& layer = history[(size_t) ((head - n + layers * 2) % layers)];
        const float age = (float) n / (float) layers;
        ribbon.clear();

        for (int i = 0; i < points; ++i)
        {
            const float u = (float) i / (float) (points - 1);
            float x, y;
            if (strip)
            {
                // trailing ribbon: older copies drift upwards and narrow slightly
                x = A.getX() + A.getWidth() * (0.5f + (u - 0.5f) * (1.0f - 0.05f * age));
                y = cy - age * A.getHeight() * 0.35f - layer[(size_t) i] * half * (1.0f - 0.5f * age) * react;
            }
            else
            {
                // waveform tunnel: older copies recede towards the vanishing point
                const float z = 1.0f / (1.0f + (age * 6.0f) * (1.0f - 0.3f * zoom));
                x = vp.x + (A.getX() + u * A.getWidth() - vp.x) * z;
                y = vp.y + (cy - vp.y) * z - layer[(size_t) i] * half * z * react;
            }
            if (i == 0) ribbon.startNewSubPath (x, y); else ribbon.lineTo (x, y);
        }

        const float fade = std::pow (1.0f - age, 1.6f);
        g.setColour (pal.b.interpolatedWith (pal.a, 0.3f * fade).withAlpha (0.45f * fade * alpha));
        g.strokePath (ribbon, fx::pen (0.8f + 0.6f * fade));
    }

    // the live trace
    trace.clear();
    const int npts = q >= 2 ? 320 : 200;
    for (int i = 0; i < npts; ++i)
    {
        const float u = (float) i / (float) (npts - 1);
        const float x = A.getX() + u * A.getWidth();
        const float y = cy - fx::sampleAt (s.wave, u) * half * react;
        if (i == 0) trace.startNewSubPath (x, y); else trace.lineTo (x, y);
    }

    // snares: two fine coloured echoes split away and rejoin
    if (split > 0.04f)
    {
        const float off = 4.0f * split + 1.0f;
        auto echo = trace;
        echo.applyTransform (juce::AffineTransform::translation (off, -off * 0.5f));
        g.setColour (pal.c.withAlpha (0.5f * split * alpha));
        g.strokePath (echo, fx::pen (0.9f));
        echo.applyTransform (juce::AffineTransform::translation (-off * 2.0f, off));
        g.setColour (pal.b.withAlpha (0.5f * split * alpha));
        g.strokePath (echo, fx::pen (0.9f));
    }

    const auto core = pal.a.interpolatedWith (juce::Colours::white, 0.15f + 0.25f * s.kick);
    fx::glow (g, trace, core.withAlpha (0.95f * alpha), pal.b.withAlpha (alpha), 1.3f + 1.2f * s.bass, 0.6f + 0.6f * s.energy, q);
}
}

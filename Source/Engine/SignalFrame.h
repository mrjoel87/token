#pragma once

#include <juce_graphics/juce_graphics.h>
#include "../AudioAnalyser.h"
#include "../Theme.h"
#include "../Parameters.h"
#include <array>
#include <cmath>

namespace token::engine
{
/**
    Everything the three worlds know about the music this frame. Built once per frame by
    TotemEngine from the real incoming audio; fixed-size arrays, so nothing is allocated per frame.
*/
struct SignalFrame
{
    static constexpr int N = 384;

    std::array<float, N> wave {};    // zero-crossing-triggered waveform, auto-gained, -1..1
    std::array<float, N> low {};     // its smooth (bass-weighted) shape
    std::array<float, N> fine {};    // the detail riding on top (wave - low)
    std::array<float, N> slow {};    // a temporally smoothed copy (flowing, for sustained sound)

    const AudioFeatures* audio = nullptr;

    // musical features, already scaled by REACT where they drive motion
    float bass = 0.0f, kick = 0.0f, snare = 0.0f, hat = 0.0f, high = 0.0f, mid = 0.0f;
    float sustain = 0.0f, quiet = 0.0f, energy = 0.0f, build = 0.0f, drop = 0.0f, breakdown = 0.0f;
    bool kickHit = false, snareHit = false, dropHit = false;

    float evo = 0.0f;        // evolution clock: SPEED-scaled, frozen by LOCK
    float motion = 0.0f;     // free clock: SPEED-scaled, never frozen (breathing, drifting light)
    double beats = 0.0;      // host song position (only meaningful when beatValid)
    bool beatValid = false;
    float beatPhase = 0.0f, barPhase = 0.0f;
    double beatsPerBar = 4.0;

    float speed = 1.0f, react = 0.65f;
    bool locked = false, strip = false;
    int quality = 2;         // 2 = full, 1 = reduced, 0 = minimal (adaptive)
    float time = 0.0f, dt = 1.0f / 60.0f;

    juce::Rectangle<float> area;
    look::Palette pal;
};

// =================================================================================================
//  Lean drawing helpers
// =================================================================================================
namespace fx
{
    constexpr float pi = juce::MathConstants<float>::pi;
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    inline juce::PathStrokeType pen (float w)
    {
        return juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    }

    inline juce::Point<float> onCircle (juce::Point<float> c, float r, float a)
    {
        return { c.x + std::sin (a) * r, c.y - std::cos (a) * r };
    }

    /** Position inside one fold of a k-fold mirror symmetry, 0..1..0 */
    inline float foldU (float theta, int folds)
    {
        const float x = theta / twoPi * (float) folds;
        const float fr = x - std::floor (x);
        return fr < 0.5f ? fr * 2.0f : (1.0f - fr) * 2.0f;
    }

    inline float sampleAt (const std::array<float, SignalFrame::N>& a, float u)
    {
        const float pos = juce::jlimit (0.0f, (float) (SignalFrame::N - 1), u * (float) (SignalFrame::N - 1));
        const int i0 = (int) pos;
        const int i1 = juce::jmin (SignalFrame::N - 1, i0 + 1);
        const float fr = pos - (float) i0;
        return a[(size_t) i0] + (a[(size_t) i1] - a[(size_t) i0]) * fr;
    }

    inline float ease (float t)
    {
        t = juce::jlimit (0.0f, 1.0f, t);
        return t * t * (3.0f - 2.0f * t);
    }

    /** Core line plus restrained halo. quality 0 = core only. */
    inline void glow (juce::Graphics& g, const juce::Path& p, juce::Colour core, juce::Colour halo,
                      float width, float amount, int quality)
    {
        if (quality >= 1 && amount > 0.02f)
        {
            g.setColour (halo.withAlpha (juce::jlimit (0.0f, 1.0f, 0.1f * amount * halo.getFloatAlpha())));
            g.strokePath (p, pen (width * 4.0f));
        }
        if (quality >= 2 && amount > 0.3f)
        {
            g.setColour (halo.withAlpha (juce::jlimit (0.0f, 1.0f, 0.18f * amount * halo.getFloatAlpha())));
            g.strokePath (p, pen (width * 2.0f));
        }
        g.setColour (core);
        g.strokePath (p, pen (width));
    }

    inline void halo (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour col)
    {
        if (r < 0.5f || col.getFloatAlpha() < 0.003f)
            return;
        juce::ColourGradient grad (col, c.x, c.y, col.withAlpha (0.0f), c.x + r, c.y, true);
        g.setGradientFill (grad);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    }

    inline void polygon (juce::Path& p, juce::Point<float> c, int sides, float r, float rot)
    {
        for (int i = 0; i < sides; ++i)
        {
            const auto v = onCircle (c, r, rot + twoPi * (float) i / (float) sides);
            if (i == 0) p.startNewSubPath (v); else p.lineTo (v);
        }
        p.closeSubPath();
    }

    inline void star (juce::Path& p, juce::Point<float> c, int n, int skip, float r, float rot)
    {
        for (int i = 0; i <= n; ++i)
        {
            const auto v = onCircle (c, r, rot + twoPi * (float) ((i * skip) % n) / (float) n);
            if (i == 0) p.startNewSubPath (v); else p.lineTo (v);
        }
        p.closeSubPath();
    }

    inline float hash (juce::uint32 seed, int i)
    {
        juce::uint32 x = seed * 747796405u + (juce::uint32) i * 2891336453u + 0x9e3779b9u;
        x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
        return (float) (x & 0xffffffu) / 16777216.0f;
    }
}
}

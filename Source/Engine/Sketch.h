#pragma once

#include <juce_graphics/juce_graphics.h>
#include <vector>
#include <cmath>
#include <numeric>

namespace token::engine
{
/**
    A drawing made of strokes (polylines). Images are sketched here in unit space (roughly -1..1,
    centre 0,0) and then drawn by the ScopePen, which turns every line into the live waveform.
    Memory is reserved once and reused every frame.
*/
struct Sketch
{
    std::vector<juce::Point<float>> pts;
    std::vector<int> starts;
    std::vector<char> closedFlags;

    Sketch()
    {
        pts.reserve (8192);
        starts.reserve (512);
        closedFlags.reserve (512);
    }

    void clear() { pts.clear(); starts.clear(); closedFlags.clear(); }
    int strokes() const noexcept { return (int) starts.size(); }
    int strokeEnd (int s) const noexcept { return s + 1 < (int) starts.size() ? starts[(size_t) s + 1] : (int) pts.size(); }

    void moveTo (juce::Point<float> p) { starts.push_back ((int) pts.size()); closedFlags.push_back (0); pts.push_back (p); }
    void moveTo (float x, float y)     { moveTo ({ x, y }); }
    void lineTo (juce::Point<float> p) { if (starts.empty()) moveTo (p); else pts.push_back (p); }
    void lineTo (float x, float y)     { lineTo ({ x, y }); }
    void close()                       { if (! closedFlags.empty()) closedFlags.back() = 1; }

    void line (juce::Point<float> a, juce::Point<float> b) { moveTo (a); lineTo (b); }
    void line (float x1, float y1, float x2, float y2)     { moveTo (x1, y1); lineTo (x2, y2); }

    static juce::Point<float> polar (juce::Point<float> c, float r, float a)
    {
        return { c.x + std::sin (a) * r, c.y - std::cos (a) * r };   // angle 0 = up, clockwise
    }

    void arc (juce::Point<float> c, float rx, float ry, float a0, float a1, int segs)
    {
        for (int i = 0; i <= segs; ++i)
        {
            const float a = a0 + (a1 - a0) * (float) i / (float) segs;
            const juce::Point<float> p (c.x + std::sin (a) * rx, c.y - std::cos (a) * ry);
            if (i == 0) moveTo (p); else lineTo (p);
        }
    }

    void circle (juce::Point<float> c, float r, int segs = 48)
    {
        arc (c, r, r, 0.0f, juce::MathConstants<float>::twoPi, segs);
        close();
    }

    void ellipse (juce::Point<float> c, float rx, float ry, int segs = 48)
    {
        arc (c, rx, ry, 0.0f, juce::MathConstants<float>::twoPi, segs);
        close();
    }

    void polygon (juce::Point<float> c, float r, int n, float rot)
    {
        for (int i = 0; i < n; ++i)
        {
            const auto p = polar (c, r, rot + juce::MathConstants<float>::twoPi * (float) i / (float) n);
            if (i == 0) moveTo (p); else lineTo (p);
        }
        close();
    }

    /** {n/skip} star polygon. If n and skip share a factor it is drawn as several closed figures. */
    void star (juce::Point<float> c, float r, int n, int skip, float rot)
    {
        const int g = std::gcd (n, skip);
        const int per = n / g;
        for (int k = 0; k < g; ++k)
        {
            for (int i = 0; i < per; ++i)
            {
                const int idx = (k + i * skip) % n;
                const auto p = polar (c, r, rot + juce::MathConstants<float>::twoPi * (float) idx / (float) n);
                if (i == 0) moveTo (p); else lineTo (p);
            }
            close();
        }
    }

    /** Almond eye between two points. */
    void eye (juce::Point<float> c, float w, float h, int segs = 24)
    {
        for (int lid = 0; lid < 2; ++lid)
        {
            const float sgn = lid == 0 ? -1.0f : 1.0f;
            for (int i = 0; i <= segs; ++i)
            {
                const float u = (float) i / (float) segs;
                const juce::Point<float> p (c.x - w + 2.0f * w * u, c.y + sgn * h * std::sin (juce::MathConstants<float>::pi * u));
                if (i == 0) moveTo (p); else lineTo (p);
            }
        }
    }
};
}

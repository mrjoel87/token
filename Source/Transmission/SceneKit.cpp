#include "SceneKit.h"
#include "../World/PixelArt.h"
#include <cmath>

namespace token::tx
{
const std::vector<SceneSpec>& sceneLibrary()
{
    static const std::vector<SceneSpec> library = []
    {
        std::vector<SceneSpec> v;
        v.reserve (160);
        addLostScenes (v);
        addOccultScenes (v);
        addHumanScenes (v);
        addNatureScenes (v);
        addRoadScenes (v);
        return v;
    }();
    return library;
}

namespace kit
{
float hash (juce::uint32 seed, int i) noexcept
{
    juce::uint32 x = seed * 747796405u + (juce::uint32) i * 2891336453u + 0x9e3779b9u;
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return (float) (x & 0xffffffu) / 16777216.0f;
}

float noise1 (juce::uint32 seed, float x) noexcept
{
    const float fl = std::floor (x);
    const int i = (int) fl;
    const float f = x - fl;
    const float s = f * f * (3.0f - 2.0f * f);
    const float a = hash (seed, i), b = hash (seed, i + 1);
    return a + (b - a) * s;
}

void sky (juce::Graphics& g, const Frame& f, juce::Colour top, juce::Colour horizon, juce::Colour bottom, float horizonFrac)
{
    juce::ColourGradient grad (top, 0.0f, f.Y (0.0f), bottom, 0.0f, f.Y (1.0f), false);
    grad.addColour (juce::jlimit (0.01, 0.99, (double) horizonFrac), horizon);
    g.setGradientFill (grad);
    g.fillRect (f.r);
}

void glow (juce::Graphics& g, juce::Point<float> c, float radius, juce::Colour col)
{
    if (radius <= 0.5f)
        return;

    juce::ColourGradient grad (col, c.x, c.y, col.withAlpha (0.0f), c.x + radius, c.y, true);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f);
}

void softEllipse (juce::Graphics& g, juce::Point<float> c, float rx, float ry, juce::Colour col)
{
    if (rx <= 0.5f || ry <= 0.5f)
        return;

    juce::Graphics::ScopedSaveState state (g);
    g.addTransform (juce::AffineTransform::scale (1.0f, ry / rx, c.x, c.y));
    glow (g, c, rx, col);
}

juce::Path ridge (const Frame& f, juce::uint32 seed, float baseFrac, float ampFrac, float jagged, int detail)
{
    juce::Path p;
    const int n = juce::jmax (8, detail);
    p.startNewSubPath (f.X (0.0f), f.Y (1.0f));

    for (int i = 0; i <= n; ++i)
    {
        const float u = (float) i / (float) n;
        float h = 0.0f, amp = 1.0f, freq = 2.0f, norm = 0.0f;

        for (int o = 0; o < 4; ++o)
        {
            float v = noise1 (seed + (juce::uint32) o * 101u, u * freq);
            if (jagged > 0.0f)
                v = juce::jmap (jagged, v, 1.0f - std::abs (v * 2.0f - 1.0f));
            h += v * amp;
            norm += amp;
            amp *= 0.5f;
            freq *= 2.2f;
        }

        h /= norm;
        p.lineTo (f.X (u), f.Y (baseFrac - h * ampFrac));
    }

    p.lineTo (f.X (1.0f), f.Y (1.0f));
    p.closeSubPath();
    return p;
}

void fogBand (juce::Graphics& g, const Frame& f, float yFrac, float hFrac, juce::Colour c, float t, juce::uint32 seed)
{
    for (int i = 0; i < 6; ++i)
    {
        const float w = f.W() * (0.35f + 0.45f * hash (seed, i));
        const float speed = 0.004f + 0.01f * hash (seed, i + 20);
        const float u = std::fmod (hash (seed, i + 10) + t * speed, 1.6f) - 0.3f;
        const float cy = f.Y (yFrac) + (hash (seed, i + 30) - 0.5f) * hFrac * f.H() * 0.6f;
        softEllipse (g, { f.X (u), cy }, w * 0.5f, f.H() * hFrac * 0.5f, c);
    }
}

void figure (juce::Graphics& g, float x, float groundY, float h, juce::Colour c, float walkPhase, bool walking)
{
    g.setColour (c);
    const float headR = h * 0.075f;
    const float shoulderY = groundY - h * 0.80f;
    const float hipY = groundY - h * 0.47f;

    g.fillEllipse (x - headR, groundY - h, headR * 2.0f, headR * 2.2f);

    juce::Path body;
    body.startNewSubPath (x - h * 0.10f, shoulderY);
    body.lineTo (x + h * 0.10f, shoulderY);
    body.lineTo (x + h * 0.08f, hipY);
    body.lineTo (x - h * 0.08f, hipY);
    body.closeSubPath();
    g.fillPath (body);

    const float swing = walking ? std::sin (walkPhase) * 0.35f : 0.0f;
    const float limbW = juce::jmax (1.0f, h * 0.055f);

    auto limb = [&] (float x0, float y0, float angle, float len)
    {
        g.drawLine (x0, y0, x0 + std::sin (angle) * len, y0 + std::cos (angle) * len, limbW);
    };

    limb (x - h * 0.04f, hipY, swing, h * 0.47f);
    limb (x + h * 0.04f, hipY, -swing, h * 0.47f);
    limb (x - h * 0.09f, shoulderY + h * 0.02f, -swing * 0.8f + 0.05f, h * 0.33f);
    limb (x + h * 0.09f, shoulderY + h * 0.02f, swing * 0.8f - 0.05f, h * 0.33f);
}

void pine (juce::Graphics& g, float x, float groundY, float h, juce::Colour c, float sway)
{
    juce::Path p;
    const int tiers = 4;

    for (int i = 0; i < tiers; ++i)
    {
        const float t0 = (float) i / (float) tiers;
        const float yb = groundY - h * (0.15f + 0.75f * t0);
        const float yt = juce::jmax (groundY - h, yb - h * 0.38f);
        const float w = h * 0.30f * (1.0f - t0 * 0.7f);
        const float s = sway * (t0 + 0.3f) * h * 0.06f;
        p.addTriangle (x - w + s * 0.5f, yb, x + w + s * 0.5f, yb, x + s, yt);
    }

    p.addRectangle (x - h * 0.025f, groundY - h * 0.16f, h * 0.05f, h * 0.16f);
    g.setColour (c);
    g.fillPath (p);
}

void trees (juce::Graphics& g, const Frame& f, juce::uint32 seed, int n, float groundFrac, float minH, float maxH,
            juce::Colour c, float t, float sway)
{
    for (int i = 0; i < n; ++i)
    {
        const float x = f.X (hash (seed, i));
        const float h = f.H() * (minH + (maxH - minH) * hash (seed, i + 500));
        const float s = std::sin (t * (0.6f + 0.5f * hash (seed, i + 900)) + (float) i) * sway;
        pine (g, x, f.Y (groundFrac) + h * 0.05f * hash (seed, i + 700), h, c, s);
    }
}

void moon (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour col)
{
    glow (g, c, r * 5.0f, col.withMultipliedAlpha (0.18f));
    g.setColour (col);
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
}

void stars (juce::Graphics& g, const Frame& f, juce::uint32 seed, int n, float maxYFrac, juce::Colour c, float t)
{
    for (int i = 0; i < n; ++i)
    {
        const float x = f.X (hash (seed, i * 3));
        const float y = f.Y (hash (seed, i * 3 + 1) * maxYFrac);
        const float s = 0.6f + 1.4f * hash (seed, i * 3 + 2) * hash (seed, i * 3 + 2);
        const float tw = 0.55f + 0.45f * std::sin (t * (0.7f + hash (seed, i + 77) * 2.0f) + (float) i);
        g.setColour (c.withMultipliedAlpha (tw));
        g.fillRect (x, y, s, s);
    }
}

void rain (juce::Graphics& g, const Frame& f, juce::uint32 seed, float t, int n, juce::Colour c, float slant)
{
    g.setColour (c);
    const float len = f.H() * 0.05f;

    for (int i = 0; i < n; ++i)
    {
        const float speed = 0.9f + 0.6f * hash (seed, i + 300);
        const float y = std::fmod (hash (seed, i + 100) + t * speed, 1.0f);
        const float x = std::fmod (hash (seed, i) + y * slant * 0.1f + 1.0f, 1.0f);
        const float px = f.X (x), py = f.Y (y);
        g.drawLine (px, py, px - slant * len * 0.3f, py - len, 0.8f);
    }
}

void snow (juce::Graphics& g, const Frame& f, juce::uint32 seed, float t, int n, juce::Colour c)
{
    g.setColour (c);

    for (int i = 0; i < n; ++i)
    {
        const float speed = 0.04f + 0.06f * hash (seed, i + 300);
        const float y = std::fmod (hash (seed, i + 100) + t * speed, 1.0f);
        const float x = hash (seed, i) + 0.02f * std::sin (t * 0.8f + (float) i);
        const float s = 1.0f + 1.5f * hash (seed, i + 600);
        g.fillEllipse (f.X (x), f.Y (y), s, s);
    }
}

void streetlight (juce::Graphics& g, float x, float groundY, float h, juce::Colour pole, juce::Colour lamp,
                  float intensity, bool cone)
{
    const juce::Point<float> head (x + h * 0.12f, groundY - h);

    if (cone)
    {
        juce::Path c;
        c.startNewSubPath (head.x - h * 0.03f, head.y);
        c.lineTo (head.x + h * 0.03f, head.y);
        c.lineTo (head.x + h * 0.45f, groundY);
        c.lineTo (head.x - h * 0.45f, groundY);
        c.closeSubPath();
        juce::ColourGradient grad (lamp.withMultipliedAlpha (0.35f * intensity), head.x, head.y,
                                   lamp.withAlpha (0.0f), head.x, groundY, false);
        g.setGradientFill (grad);
        g.fillPath (c);
        softEllipse (g, { head.x, groundY }, h * 0.5f, h * 0.06f, lamp.withMultipliedAlpha (0.35f * intensity));
    }

    glow (g, head, h * 0.35f, lamp.withMultipliedAlpha (0.6f * intensity));

    g.setColour (pole);
    g.drawLine (x, groundY, x, groundY - h, juce::jmax (1.0f, h * 0.025f));
    g.drawLine (x, groundY - h, head.x, head.y, juce::jmax (1.0f, h * 0.02f));
    g.setColour (lamp.withMultipliedAlpha (juce::jlimit (0.0f, 1.0f, intensity)));
    g.fillEllipse (head.x - h * 0.03f, head.y - h * 0.01f, h * 0.06f, h * 0.03f);
}

void reflection (juce::Graphics& g, float x, float y, float width, float depth, juce::Colour c, float t, juce::uint32 seed)
{
    const int n = 14;
    for (int i = 0; i < n; ++i)
    {
        const float fi = (float) i / (float) n;
        const float yy = y + depth * fi;
        const float w = width * (0.3f + 0.9f * noise1 (seed + (juce::uint32) i, t * 1.5f)) * (1.0f - fi * 0.4f);
        const float dx = std::sin (t * 1.7f + (float) i * 1.3f) * width * 0.15f;
        g.setColour (c.withMultipliedAlpha (1.0f - fi));
        g.fillRect (x - w * 0.5f + dx, yy, w, juce::jmax (1.0f, depth / (float) n * 0.45f));
    }
}

void glowStroke (juce::Graphics& g, const juce::Path& p, juce::Colour c, float thickness, float glowAmount)
{
    pixel::drawGlowPath (g, p, c, glowAmount, thickness);
}

void glyph (juce::Graphics& g, const char* spec, juce::Rectangle<float> box, juce::Colour c, float thickness, float glowAmount)
{
    auto p = pixel::unitPath (spec);
    p.applyTransform (juce::AffineTransform::scale (box.getWidth(), box.getHeight()).translated (box.getX(), box.getY()));
    glowStroke (g, p, c, thickness, glowAmount);
}

void sevenSegment (juce::Graphics& g, int digit, juce::Rectangle<float> b, juce::Colour c, float thickness)
{
    static const int masks[] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };
    const int m = masks[juce::jlimit (0, 9, digit)];
    const float x0 = b.getX(), x1 = b.getRight(), y0 = b.getY(), y1 = b.getBottom(), ym = b.getCentreY();

    g.setColour (c);
    if (m & 0x01) g.drawLine (x0, y0, x1, y0, thickness);
    if (m & 0x02) g.drawLine (x1, y0, x1, ym, thickness);
    if (m & 0x04) g.drawLine (x1, ym, x1, y1, thickness);
    if (m & 0x08) g.drawLine (x0, y1, x1, y1, thickness);
    if (m & 0x10) g.drawLine (x0, ym, x0, y1, thickness);
    if (m & 0x20) g.drawLine (x0, y0, x0, ym, thickness);
    if (m & 0x40) g.drawLine (x0, ym, x1, ym, thickness);
}

void bird (juce::Graphics& g, float x, float y, float size, float flap, juce::Colour c)
{
    const float wing = std::sin (flap) * size * 0.45f;
    juce::Path p;
    p.startNewSubPath (x - size, y - wing);
    p.quadraticTo (x - size * 0.4f, y - wing * 0.3f - size * 0.1f, x, y);
    p.quadraticTo (x + size * 0.4f, y - wing * 0.3f - size * 0.1f, x + size, y - wing);
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (juce::jmax (1.0f, size * 0.18f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void cloud (juce::Graphics& g, float cx, float cy, float w, float h, juce::Colour body, juce::Colour rim, juce::uint32 seed)
{
    const int n = 7;
    for (int i = 0; i < n; ++i)
    {
        const float u = (float) i / (float) (n - 1) - 0.5f;
        const float r = h * (0.45f + 0.45f * hash (seed, i)) * (1.0f - std::abs (u) * 0.8f);
        const float x = cx + u * w * 0.85f;
        const float y = cy - r * 0.3f;
        glow (g, { x, y - r * 0.25f }, r * 1.15f, rim);
        g.setColour (body);
        g.fillEllipse (x - r, y - r * 0.8f, r * 2.0f, r * 1.6f);
    }
}
}
}

#include "ArtKit.h"
#include <cmath>

namespace token::art
{
const std::vector<Spec>& library()
{
    static const std::vector<Spec> lib = []
    {
        std::vector<Spec> v;
        v.reserve (160);
        addAncient (v);
        addNature (v);
        addCelestial (v);
        addSentimental (v);
        addSurreal (v);
        addGeometric (v);
        return v;
    }();
    return lib;
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

float noise (juce::uint32 seed, float x) noexcept
{
    const float fl = std::floor (x);
    const int i = (int) fl;
    const float fr = x - fl;
    const float s = fr * fr * (3.0f - 2.0f * fr);
    const float a = hash (seed, i), b = hash (seed, i + 1);
    return a + (b - a) * s;
}

float ease (float t) noexcept
{
    t = juce::jlimit (0.0f, 1.0f, t);
    return t * t * (3.0f - 2.0f * t);
}

juce::PathStrokeType pen (float width)
{
    return juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
}

void halo (juce::Graphics& g, juce::Point<float> c, float radius, juce::Colour col)
{
    if (radius <= 0.5f)
        return;
    juce::ColourGradient grad (col, c.x, c.y, col.withAlpha (0.0f), c.x + radius, c.y, true);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f);
}

void blob (juce::Graphics& g, juce::Point<float> c, float rx, float ry, juce::Colour col)
{
    if (rx <= 0.5f || ry <= 0.5f)
        return;
    juce::Graphics::ScopedSaveState state (g);
    g.addTransform (juce::AffineTransform::scale (1.0f, ry / rx, c.x, c.y));
    halo (g, c, rx, col);
}

void glowLine (juce::Graphics& g, const juce::Path& p, juce::Colour core, juce::Colour glowColour,
               float width, float glowAmount, bool lightTheme)
{
    const float gs = lightTheme ? 0.45f : 1.0f;
    if (glowAmount > 0.01f)
    {
        g.setColour (glowColour.withAlpha (juce::jlimit (0.0f, 1.0f, 0.06f * glowAmount * gs)));
        g.strokePath (p, pen (width * 6.0f));
        g.setColour (glowColour.withAlpha (juce::jlimit (0.0f, 1.0f, 0.16f * glowAmount * gs)));
        g.strokePath (p, pen (width * 2.6f));
    }
    g.setColour (core);
    g.strokePath (p, pen (width));
}

void silhouette (juce::Graphics& g, const juce::Path& p, const Frame& f, float outlineGlow)
{
    g.setColour (f.ink.sil);
    g.fillPath (p);
    glowLine (g, p, f.ink.line.withAlpha (f.ink.light ? 0.55f : 0.7f), f.ink.glow, 0.8f, outlineGlow, f.ink.light);
}

juce::Path ridge (const Frame& f, juce::uint32 seed, float baseV, float ampV, float jagged, int detail)
{
    juce::Path p;
    const int n = juce::jmax (8, detail);
    p.startNewSubPath (f.box.getX(), f.box.getBottom());

    for (int i = 0; i <= n; ++i)
    {
        const float u = (float) i / (float) n;
        float h = 0.0f, amp = 1.0f, freq = 2.0f, norm = 0.0f;
        for (int o = 0; o < 4; ++o)
        {
            float v = noise (seed + (juce::uint32) o * 101u, u * freq);
            if (jagged > 0.0f)
                v = juce::jmap (jagged, v, 1.0f - std::abs (v * 2.0f - 1.0f));
            h += v * amp;
            norm += amp;
            amp *= 0.5f;
            freq *= 2.2f;
        }
        h /= norm;
        p.lineTo (f.box.getX() + u * f.W(), f.Y (baseV - h * ampV));
    }

    p.lineTo (f.box.getRight(), f.box.getBottom());
    p.closeSubPath();
    return p;
}

juce::Path polygon (juce::Point<float> c, int sides, float radius, float rotation)
{
    juce::Path p;
    for (int i = 0; i < sides; ++i)
    {
        const float a = rotation + juce::MathConstants<float>::twoPi * (float) i / (float) sides;
        const juce::Point<float> v (c.x + std::sin (a) * radius, c.y - std::cos (a) * radius);
        if (i == 0) p.startNewSubPath (v); else p.lineTo (v);
    }
    p.closeSubPath();
    return p;
}

juce::Path unitPath (const char* spec, juce::Rectangle<float> box)
{
    juce::Path p;
    for (const auto& strokeText : juce::StringArray::fromTokens (spec, "|", ""))
    {
        bool first = true;
        for (const auto& pt : juce::StringArray::fromTokens (strokeText, " ", ""))
        {
            if (pt.isEmpty())
                continue;
            const float x = box.getX() + box.getWidth()  * pt.upToFirstOccurrenceOf (",", false, false).getFloatValue() / 100.0f;
            const float y = box.getY() + box.getHeight() * pt.fromFirstOccurrenceOf (",", false, false).getFloatValue() / 100.0f;
            if (first) { p.startNewSubPath (x, y); first = false; }
            else       { p.lineTo (x, y); }
        }
    }
    return p;
}

void fog (juce::Graphics& g, const Frame& f, float yV, float hV, juce::Colour col)
{
    for (int i = 0; i < 5; ++i)
    {
        const float w = f.W() * (0.35f + 0.4f * hash (f.seed, i + 40));
        const float speed = 0.005f + 0.01f * hash (f.seed, i + 60);
        const float u = std::fmod (hash (f.seed, i + 80) + f.t * speed, 1.5f) - 0.25f;
        const float cy = f.Y (yV + (hash (f.seed, i + 90) - 0.5f) * hV * 0.6f);
        blob (g, { f.box.getX() + u * f.W(), cy }, w * 0.5f, f.H() * hV * 0.5f, col);
    }
}

void pine (juce::Graphics& g, float x, float groundY, float h, juce::Colour col, float sway)
{
    juce::Path p;
    for (int i = 0; i < 4; ++i)
    {
        const float t0 = (float) i / 4.0f;
        const float yb = groundY - h * (0.15f + 0.75f * t0);
        const float yt = juce::jmax (groundY - h, yb - h * 0.38f);
        const float w = h * 0.28f * (1.0f - t0 * 0.7f);
        const float s = sway * (t0 + 0.3f) * h * 0.06f;
        p.addTriangle (x - w + s * 0.5f, yb, x + w + s * 0.5f, yb, x + s, yt);
    }
    p.addRectangle (x - h * 0.02f, groundY - h * 0.16f, h * 0.04f, h * 0.16f);
    g.setColour (col);
    g.fillPath (p);
}

void pixelRect (juce::Graphics& g, float x, float y, float w, float h, juce::Colour col)
{
    const int x0 = juce::roundToInt (x), y0 = juce::roundToInt (y);
    const int x1 = juce::roundToInt (x + w), y1 = juce::roundToInt (y + h);
    g.setColour (col);
    g.fillRect (x0, y0, juce::jmax (1, x1 - x0), juce::jmax (1, y1 - y0));
}

void person (juce::Graphics& g, float x, float gy, float px, juce::Colour col, Pose pose, float phase, float facing)
{
    auto R = [&] (float ux, float uy, float uw, float uh) { pixelRect (g, x + ux * px, gy + uy * px, uw * px, uh * px, col); };
    const float fc = facing >= 0.0f ? 1.0f : -1.0f;

    if (pose == Pose::Sit)
    {
        R (-1.0f, -7.0f, 2.0f, 2.0f);                      // head
        R (-1.0f, -5.0f, 2.0f, 3.0f);                      // body
        R (fc > 0.0f ? -1.0f : -2.0f, -2.0f, 3.0f, 1.0f);  // thighs
        R (fc > 0.0f ? 1.0f : -2.0f, -2.0f, 1.0f, 2.0f);   // shins
        return;
    }

    R (-1.0f, -9.0f, 2.0f, 2.0f);   // head
    R (-1.0f, -7.0f, 2.0f, 4.0f);   // body

    if (pose == Pose::Walk && (((int) std::floor (phase)) % 2) != 0)
    {
        R (-2.0f, -3.0f, 1.0f, 3.0f);
        R (1.0f, -3.0f, 1.0f, 3.0f);
        R (-2.0f, -7.0f, 1.0f, 2.0f);
        R (1.0f, -6.0f, 1.0f, 2.0f);
    }
    else
    {
        R (-1.0f, -3.0f, 1.0f, 3.0f);
        R (0.0f, -3.0f, 1.0f, 3.0f);
        R (-2.0f, -6.0f, 1.0f, 3.0f);
        R (1.0f, -6.0f, 1.0f, 3.0f);
    }
}

void stars (juce::Graphics& g, const Frame& f, juce::uint32 seed, int n, float maxV, juce::Colour col, float twinkle)
{
    for (int i = 0; i < n; ++i)
    {
        const float x = f.box.getX() + hash (seed, i * 3) * f.W();
        const float y = f.Y (hash (seed, i * 3 + 1) * maxV);
        const float s = 0.7f + 1.3f * hash (seed, i * 3 + 2) * hash (seed, i * 3 + 2);
        const float tw = 0.5f + 0.5f * std::sin (f.t * (0.8f + 2.0f * hash (seed, i + 500)) + (float) i);
        g.setColour (col.withMultipliedAlpha (juce::jlimit (0.0f, 1.0f, 0.35f + 0.45f * tw + 0.4f * twinkle * hash (seed, i + 900))));
        g.fillRect (x, y, s, s);
    }
}

void rain (juce::Graphics& g, const Frame& f, juce::uint32 seed, int n, juce::Colour col, float slant, float topV)
{
    g.setColour (col);
    const float len = f.H() * 0.06f;
    for (int i = 0; i < n; ++i)
    {
        const float speed = 0.9f + 0.6f * hash (seed, i + 300);
        const float v = std::fmod (hash (seed, i + 100) + f.t * speed, 1.0f);
        const float x = f.box.getX() + hash (seed, i) * f.W() + slant * v * f.H() * 0.15f;
        const float y = f.Y (topV + (1.0f - topV) * v);
        g.drawLine (x, y, x - slant * len * 0.3f, y - len, 0.7f);
    }
}

void snow (juce::Graphics& g, const Frame& f, juce::uint32 seed, int n, juce::Colour col)
{
    g.setColour (col);
    for (int i = 0; i < n; ++i)
    {
        const float speed = 0.04f + 0.06f * hash (seed, i + 300);
        const float v = std::fmod (hash (seed, i + 100) + f.t * speed, 1.0f);
        const float x = f.box.getX() + (hash (seed, i) + 0.02f * std::sin (f.t * 0.8f + (float) i)) * f.W();
        const float s = 1.0f + 1.4f * hash (seed, i + 600);
        g.fillEllipse (x, f.Y (v), s, s);
    }
}

void cloud (juce::Graphics& g, float cx, float cy, float w, float h, juce::Colour body, juce::Colour rim, juce::uint32 seed)
{
    for (int i = 0; i < 6; ++i)
    {
        const float u = (float) i / 5.0f - 0.5f;
        const float r = h * (0.45f + 0.45f * hash (seed, i)) * (1.0f - std::abs (u) * 0.8f);
        const float x = cx + u * w * 0.85f;
        const float y = cy - r * 0.3f;
        halo (g, { x, y - r * 0.25f }, r * 1.2f, rim);
        g.setColour (body);
        g.fillEllipse (x - r, y - r * 0.8f, r * 2.0f, r * 1.6f);
    }
}

void bird (juce::Graphics& g, float x, float y, float size, float flap, juce::Colour col)
{
    const float wing = std::sin (flap) * size * 0.45f;
    juce::Path p;
    p.startNewSubPath (x - size, y - wing);
    p.quadraticTo (x - size * 0.4f, y - wing * 0.3f - size * 0.1f, x, y);
    p.quadraticTo (x + size * 0.4f, y - wing * 0.3f - size * 0.1f, x + size, y - wing);
    g.setColour (col);
    g.strokePath (p, pen (juce::jmax (0.8f, size * 0.16f)));
}

void lamp (juce::Graphics& g, float x, float gy, float h, juce::Colour pole, juce::Colour light, float intensity)
{
    const juce::Point<float> head (x + h * 0.12f, gy - h);
    juce::Path cone;
    cone.startNewSubPath (head.x - h * 0.03f, head.y);
    cone.lineTo (head.x + h * 0.03f, head.y);
    cone.lineTo (head.x + h * 0.42f, gy);
    cone.lineTo (head.x - h * 0.42f, gy);
    cone.closeSubPath();
    juce::ColourGradient grad (light.withMultipliedAlpha (0.3f * intensity), head.x, head.y, light.withAlpha (0.0f), head.x, gy, false);
    g.setGradientFill (grad);
    g.fillPath (cone);
    halo (g, head, h * 0.3f, light.withMultipliedAlpha (0.55f * intensity));

    g.setColour (pole);
    g.drawLine (x, gy, x, gy - h, juce::jmax (1.0f, h * 0.02f));
    g.drawLine (x, gy - h, head.x, head.y, juce::jmax (1.0f, h * 0.015f));
    g.setColour (light.withMultipliedAlpha (juce::jlimit (0.0f, 1.0f, intensity)));
    g.fillEllipse (head.x - h * 0.025f, head.y - h * 0.01f, h * 0.05f, h * 0.025f);
}

void moon (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour col, float haloAmount)
{
    halo (g, c, r * 4.5f, col.withMultipliedAlpha (0.16f * haloAmount));
    g.setColour (col);
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
}

void reflection (juce::Graphics& g, float x, float y, float width, float depth, juce::Colour col, float t, juce::uint32 seed)
{
    for (int i = 0; i < 12; ++i)
    {
        const float fi = (float) i / 12.0f;
        const float w = width * (0.3f + 0.9f * noise (seed + (juce::uint32) i, t * 1.5f)) * (1.0f - fi * 0.4f);
        const float dx = std::sin (t * 1.7f + (float) i * 1.3f) * width * 0.15f;
        g.setColour (col.withMultipliedAlpha (1.0f - fi));
        g.fillRect (x - w * 0.5f + dx, y + depth * fi, w, juce::jmax (1.0f, depth / 12.0f * 0.45f));
    }
}

void sevenSeg (juce::Graphics& g, int digit, juce::Rectangle<float> b, juce::Colour col, float thickness)
{
    static const int masks[] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };
    const int m = masks[juce::jlimit (0, 9, digit)];
    const float x0 = b.getX(), x1 = b.getRight(), y0 = b.getY(), y1 = b.getBottom(), ym = b.getCentreY();
    g.setColour (col);
    if ((m & 0x01) != 0) g.drawLine (x0, y0, x1, y0, thickness);
    if ((m & 0x02) != 0) g.drawLine (x1, y0, x1, ym, thickness);
    if ((m & 0x04) != 0) g.drawLine (x1, ym, x1, y1, thickness);
    if ((m & 0x08) != 0) g.drawLine (x0, y1, x1, y1, thickness);
    if ((m & 0x10) != 0) g.drawLine (x0, ym, x0, y1, thickness);
    if ((m & 0x20) != 0) g.drawLine (x0, y0, x0, ym, thickness);
    if ((m & 0x40) != 0) g.drawLine (x0, ym, x1, ym, thickness);
}
}
}

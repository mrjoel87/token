// =================================================================================================
//  NATURE & WEATHER
// =================================================================================================
#include "ArtKit.h"
#include <cmath>

namespace token::art
{
namespace
{
namespace K = kit;
using G = juce::Graphics;
using Pt = juce::Point<float>;
using Rc = juce::Rectangle<float>;
constexpr float pi = juce::MathConstants<float>::pi;
constexpr float twoPi = juce::MathConstants<float>::twoPi;

float lineW (const Frame& f, float k) { return juce::jmax (0.8f, f.S() * k); }

// ------------------------------------------------------------------------------------------------
void whale (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float L = f.H() * 2.2f;
    const float dir = f.flip ? -1.0f : 1.0f;
    const float travel = (f.W() + L * 1.2f) * f.life;
    const float cx = f.flip ? f.box.getRight() + L * 0.6f - travel : f.box.getX() - L * 0.6f + travel;
    const float cy = f.Y (0.5f) + std::sin (f.t * 0.4f) * f.H() * 0.12f;
    const float tail = std::sin (f.t * (1.0f + f.bass)) * L * 0.04f;

    juce::Path p;
    p.startNewSubPath (L * 0.5f, 0.0f);
    p.cubicTo (L * 0.45f, -L * 0.14f, L * 0.05f, -L * 0.17f, -L * 0.30f, -L * 0.07f);
    p.cubicTo (-L * 0.40f, -L * 0.04f, -L * 0.45f, -L * 0.02f + tail, -L * 0.50f, -L * 0.09f + tail);
    p.lineTo (-L * 0.56f, -L * 0.12f + tail);
    p.lineTo (-L * 0.53f, tail);
    p.lineTo (-L * 0.58f, L * 0.07f + tail);
    p.lineTo (-L * 0.48f, L * 0.02f + tail);
    p.cubicTo (-L * 0.30f, L * 0.06f, L * 0.10f, L * 0.12f, L * 0.5f, 0.0f);
    p.closeSubPath();
    p.startNewSubPath (L * 0.05f, L * 0.06f);
    p.quadraticTo (L * 0.0f, L * 0.16f, -L * 0.08f, L * 0.14f);
    p.applyTransform (juce::AffineTransform::scale (dir, 1.0f).rotated (dir * 0.05f * std::sin (f.t * 0.4f)).translated (cx, cy));

    K::halo (g, { cx, cy }, L * 0.5f, k.haze.withAlpha (0.12f + 0.12f * f.bass));
    K::silhouette (g, p, f, 0.4f + 0.6f * f.bass);

    g.setColour (k.line.withAlpha (0.35f));
    for (int i = 0; i < 8; ++i)
    {
        const float rise = std::fmod (f.t * 0.2f + K::hash (f.seed, i), 1.0f);
        const float bx = cx + dir * L * (0.2f - 0.1f * (float) i) + std::sin (f.t + (float) i) * 3.0f;
        g.drawEllipse (bx, cy - L * 0.1f - rise * f.H() * 0.4f, 3.0f, 3.0f, 0.6f);
    }
}

void koi (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.S() * 0.26f;
    for (int i = 1; i <= 3; ++i)
    {
        const float rr = r * (0.6f + 0.5f * (float) i) + std::fmod (f.t * 8.0f, 20.0f);
        g.setColour (k.line.withAlpha (0.12f));
        g.drawEllipse (c.x - rr, c.y - rr * 0.5f, rr * 2.0f, rr, 0.6f);
    }

    for (int fish = 0; fish < 2; ++fish)
    {
        const float a = f.t * (0.35f + 0.2f * f.mid) + (float) fish * pi;
        const Pt head (c.x + std::sin (a) * r, c.y - std::cos (a) * r * 0.55f);
        const float heading = a + pi * 0.5f;
        juce::Path body;
        const float len = f.S() * 0.22f;
        body.startNewSubPath (0.0f, -len * 0.5f);
        body.quadraticTo (len * 0.16f, -len * 0.1f, 0.0f, len * 0.35f);
        body.quadraticTo (-len * 0.16f, -len * 0.1f, 0.0f, -len * 0.5f);
        body.closeSubPath();
        const float sw = std::sin (f.t * 4.0f + (float) fish) * len * 0.12f;
        body.startNewSubPath (0.0f, len * 0.3f);
        body.lineTo (sw - len * 0.1f, len * 0.55f);
        body.lineTo (sw + len * 0.1f, len * 0.55f);
        body.closeSubPath();
        body.applyTransform (juce::AffineTransform::rotation (heading).translated (head.x, head.y));
        g.setColour ((fish == 0 ? k.accent : k.line).withAlpha (0.75f));
        g.fillPath (body);
        K::glowLine (g, body, k.line.withAlpha (0.5f), k.glow, 0.7f, 0.4f, k.light);
    }
}

void jellyfish (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float cx = f.X (0.5f) + std::sin (f.t * 0.4f) * f.W() * 0.08f;
    const float cy = f.Y (0.75f - 0.45f * f.life);
    const float s = f.S() * 0.32f;
    const float pulse = 1.0f + 0.12f * std::sin (f.t * 2.2f) + 0.15f * f.kick;

    K::halo (g, { cx, cy }, s * 1.8f, k.glow.withAlpha (0.18f + 0.2f * f.bass));
    juce::Path bell;
    bell.addCentredArc (cx, cy, s * pulse, s * 0.75f, 0.0f, -pi * 0.5f, pi * 0.5f, true);
    bell.closeSubPath();
    g.setColour (k.line.withAlpha (k.light ? 0.15f : 0.2f));
    g.fillPath (bell);
    K::glowLine (g, bell, k.line.withAlpha (0.8f), k.glow, 0.9f, 0.6f, k.light);

    for (int t = 0; t < 6; ++t)
    {
        juce::Path tent;
        const float tx = cx - s * 0.75f + s * 0.3f * (float) t;
        tent.startNewSubPath (tx, cy);
        tent.cubicTo (tx + std::sin (f.t * 1.6f + (float) t) * s * 0.3f, cy + s * 0.8f,
                      tx - std::sin (f.t * 1.3f + (float) t) * s * 0.3f, cy + s * 1.6f,
                      tx + std::sin (f.t + (float) t) * s * 0.2f, cy + s * 2.4f);
        g.setColour (k.line.withAlpha (0.35f));
        g.strokePath (tent, K::pen (0.7f));
    }
}

void flock (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float travel = f.life * 1.5f - 0.25f;
    for (int i = 0; i < 22; ++i)
    {
        const float u = travel - 0.25f * K::hash (f.seed, i) + 0.01f * std::sin (f.t * 0.7f + (float) i);
        const float v = 0.2f + 0.6f * K::hash (f.seed, i + 50) + 0.04f * std::sin (f.t * 0.5f + (float) i * 0.7f);
        K::bird (g, f.X (u), f.Y (v), f.H() * (0.03f + 0.025f * K::hash (f.seed, i + 90)),
                 f.t * (6.0f + 4.0f * f.high) + (float) i * 1.7f, k.line.withAlpha (0.8f));
    }
}

void geese (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float lead = f.life * 1.5f - 0.2f;
    for (int i = 0; i < 11; ++i)
    {
        const int side = (i % 2) * 2 - 1;
        const float rank = (float) ((i + 1) / 2);
        const float u = lead - rank * 0.03f;
        const float v = 0.5f + (float) side * rank * 0.07f;
        K::bird (g, f.X (u), f.Y (v), f.H() * 0.05f, f.t * 4.0f + (float) i * 0.5f, k.line.withAlpha (0.85f));
    }
}

void ravens (G& g, const Frame& f)
{
    const auto& k = f.ink;
    for (int i = 0; i < 4; ++i)
    {
        const float u = f.life * 1.4f - 0.2f - 0.12f * (float) i;
        const float x = f.X (u), y = f.Y (0.45f + 0.18f * K::hash (f.seed, i) + 0.05f * std::sin (f.t + (float) i));
        const float px = juce::jmax (1.5f, f.px * 1.2f);
        const bool up = std::fmod (f.t * 2.5f + (float) i * 0.3f, 1.0f) < 0.5f;
        const auto col = k.light ? k.sil : k.line.withAlpha (0.85f);
        K::pixelRect (g, x - px, y, px * 3.0f, px, col);
        K::pixelRect (g, x + px * 2.0f, y - px * 0.5f, px, px, col);
        if (up)
        {
            K::pixelRect (g, x - px * 3.0f, y - px * 2.0f, px * 2.0f, px, col);
            K::pixelRect (g, x + px * 0.5f, y - px * 2.0f, px * 2.0f, px, col);
        }
        else
        {
            K::pixelRect (g, x - px * 3.0f, y + px, px * 2.0f, px, col);
            K::pixelRect (g, x + px * 0.5f, y + px, px * 2.0f, px, col);
        }
    }
}

void rainCloud (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float drift = (f.life - 0.5f) * f.W() * 0.15f;
    const float swell = 1.0f + 0.12f * f.bass;
    K::rain (g, f, f.seed, 40 + (int) (60.0f * f.high), k.line.withAlpha (0.4f + 0.3f * f.high), 0.2f, 0.35f);
    K::cloud (g, f.X (0.5f) + drift, f.Y (0.25f), f.W() * 0.75f * swell, f.H() * 0.28f * swell,
              k.light ? k.sil.withAlpha (0.85f) : k.sil.interpolatedWith (k.line, 0.18f), k.haze.withAlpha (0.15f), f.seed);
}

void storm (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const bool flash = f.kick > 0.55f || std::fmod (f.t, 7.0f) < 0.07f;
    if (flash)
        K::halo (g, f.P (0.5f, 0.4f), f.S() * 1.2f, k.line.withAlpha (0.18f));

    for (int i = 0; i < 4; ++i)
    {
        const float u = 0.15f + 0.24f * (float) i + 0.03f * std::sin (f.t * 0.1f + (float) i);
        K::cloud (g, f.X (u), f.Y (0.3f + 0.08f * K::hash (f.seed, i)), f.W() * 0.35f, f.H() * 0.3f,
                  k.light ? k.sil.withAlpha (0.8f) : k.sil.interpolatedWith (k.line, 0.12f),
                  k.haze.withAlpha (flash ? 0.4f : 0.08f), f.seed + (juce::uint32) i);
    }

    if (flash)
    {
        juce::Path bolt;
        float x = f.X (0.3f + 0.4f * K::hash (f.seed, (int) (f.t * 0.3f))), y = f.Y (0.45f);
        bolt.startNewSubPath (x, y);
        for (int i = 0; i < 7; ++i)
        {
            x += (K::hash (f.seed, i + (int) f.t) - 0.5f) * f.W() * 0.08f;
            y += f.H() * 0.075f;
            bolt.lineTo (x, y);
        }
        K::glowLine (g, bolt, k.line, k.glow, 1.2f, 1.0f, k.light);
    }
}

void cirrus (G& g, const Frame& f)
{
    const auto& k = f.ink;
    for (int l = 0; l < 7; ++l)
    {
        juce::Path wisp;
        const float v = 0.15f + 0.11f * (float) l;
        const float drift = std::fmod (f.t * 0.01f * (1.0f + 0.3f * (float) l) + K::hash (f.seed, l), 1.0f) * 0.3f;
        for (int i = 0; i <= 40; ++i)
        {
            const float u = 0.05f + 0.6f * (float) i / 40.0f + drift + 0.15f * K::hash (f.seed, l + 9);
            const float y = f.Y (v) + std::sin (u * 9.0f + (float) l) * f.H() * 0.03f;
            if (i == 0) wisp.startNewSubPath (f.X (u), y); else wisp.lineTo (f.X (u), y);
        }
        g.setColour (k.line.withAlpha (0.15f + 0.15f * f.flow));
        g.strokePath (wisp, K::pen (0.8f + 1.5f * K::hash (f.seed, l + 30)));
    }
}

void snowPeaks (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto range = K::ridge (f, f.seed, 0.95f, 0.75f, 0.9f, 70);
    K::silhouette (g, range, f, 0.3f);
    {
        G::ScopedSaveState state (g);
        g.reduceClipRegion (Rc (f.box.getX(), f.box.getY(), f.W(), f.H() * 0.45f).toNearestInt());
        g.setColour (k.line.withAlpha (k.light ? 0.25f : 0.35f));
        g.fillPath (range);
    }
    K::snow (g, f, f.seed, 60, k.line.withAlpha (0.5f));
}

void fogRanges (G& g, const Frame& f)
{
    const auto& k = f.ink;
    for (int l = 0; l < 3; ++l)
    {
        const auto r = K::ridge (f, f.seed + (juce::uint32) l * 7u, 0.55f + 0.18f * (float) l, 0.4f - 0.08f * (float) l, 0.6f, 60);
        g.setColour (k.sil.interpolatedWith (k.bg, 0.6f - 0.25f * (float) l));
        g.fillPath (r);
        K::glowLine (g, r, k.line.withAlpha (0.25f + 0.1f * (float) l), k.glow, 0.7f, 0.2f, k.light);
        K::fog (g, f, 0.62f + 0.14f * (float) l, 0.16f, k.haze.withAlpha (0.18f + 0.12f * f.flow));
    }
}

void pineForest (G& g, const Frame& f)
{
    const auto& k = f.ink;
    for (int row = 0; row < 2; ++row)
    {
        const int n = row == 0 ? 22 : 12;
        for (int i = 0; i < n; ++i)
        {
            const float u = K::hash (f.seed + (juce::uint32) row * 77u, i);
            const float h = f.H() * (row == 0 ? 0.45f : 0.85f) * (0.6f + 0.4f * K::hash (f.seed, i + 300 + row * 40));
            const float sway = std::sin (f.t * (0.6f + 0.4f * K::hash (f.seed, i + 500)) + (float) i) * (0.3f + 1.2f * f.bass);
            K::pine (g, f.X (u), f.box.getBottom(), h, row == 0 ? k.sil.interpolatedWith (k.bg, 0.4f) : k.sil, sway);
        }
        if (row == 0)
            K::fog (g, f, 0.75f, 0.2f, k.haze.withAlpha (0.2f));
    }
}

void moonSea (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float horizon = 0.55f;
    const auto m = f.P (0.5f, 0.25f);
    K::moon (g, m, f.S() * 0.09f, k.line, 1.2f);
    g.setColour (k.line.withAlpha (0.3f));
    g.drawLine (f.box.getX(), f.Y (horizon), f.box.getRight(), f.Y (horizon), 0.7f);

    for (int i = 0; i < 34; ++i)
    {
        const float d = (float) i / 34.0f;
        const float y = f.Y (horizon) + f.H() * (1.0f - horizon) * d * d;
        const float w = f.W() * (0.02f + 0.12f * d) * (0.4f + 0.8f * K::noise (f.seed + (juce::uint32) i, f.t * 1.2f));
        const float x = m.x + std::sin (f.t * 1.1f + (float) i * 0.9f) * f.W() * 0.02f * d;
        g.setColour (k.line.withAlpha ((0.6f - 0.4f * d) * (0.5f + 0.5f * f.high)));
        g.fillRect (x - w * 0.5f, y, w, juce::jmax (1.0f, 2.0f * d + 0.5f));
    }
}

void waves (G& g, const Frame& f)
{
    const auto& k = f.ink;
    for (int i = 0; i < 12; ++i)
    {
        const float d = std::fmod ((float) i / 12.0f + f.t * 0.03f, 1.0f);
        const float y = f.Y (d * d);
        juce::Path crest;
        for (int j = 0; j <= 40; ++j)
        {
            const float u = (float) j / 40.0f;
            const float yy = y + std::sin (u * 9.0f + f.t + (float) i) * f.H() * 0.03f * (0.3f + d) * (0.5f + f.bass);
            if (j == 0) crest.startNewSubPath (f.X (u), yy); else crest.lineTo (f.X (u), yy);
        }
        g.setColour (k.line.withAlpha (0.1f + 0.45f * d));
        g.strokePath (crest, K::pen (0.6f + 1.6f * d));
    }
}

void sakura (G& g, const Frame& f)
{
    const auto& k = f.ink;
    juce::Path branch;
    branch.startNewSubPath (f.P (0.0f, 0.15f));
    branch.cubicTo (f.P (0.25f, 0.2f), f.P (0.4f, 0.1f), f.P (0.7f, 0.3f));
    branch.startNewSubPath (f.P (0.3f, 0.17f));
    branch.quadraticTo (f.P (0.4f, 0.35f), f.P (0.55f, 0.45f));
    branch.startNewSubPath (f.P (0.5f, 0.2f));
    branch.quadraticTo (f.P (0.6f, 0.05f), f.P (0.8f, 0.08f));
    g.setColour (k.sil.interpolatedWith (k.line, 0.25f));
    g.strokePath (branch, K::pen (lineW (f, 0.012f)));

    auto blossom = [&] (Pt c, float r, float a)
    {
        juce::Path p;
        for (int i = 0; i < 5; ++i)
        {
            const float ang = a + (float) i / 5.0f * twoPi;
            p.addEllipse (c.x + std::sin (ang) * r * 0.6f - r * 0.4f, c.y - std::cos (ang) * r * 0.6f - r * 0.4f, r * 0.8f, r * 0.8f);
        }
        g.setColour (k.accent.withAlpha (0.55f));
        g.fillPath (p);
    };

    for (int i = 0; i < 14; ++i)
    {
        const float u = 0.15f + 0.65f * K::hash (f.seed, i);
        const float v = 0.08f + 0.35f * K::hash (f.seed, i + 20);
        blossom (f.P (u, v), f.S() * (0.035f + 0.02f * K::hash (f.seed, i + 40)), f.t * 0.1f + (float) i);
    }

    for (int i = 0; i < 12; ++i)
    {
        const float fall = std::fmod (f.t * (0.05f + 0.04f * K::hash (f.seed, i + 60)) + K::hash (f.seed, i + 70), 1.0f);
        const float u = 0.2f + 0.7f * K::hash (f.seed, i + 80) + 0.05f * std::sin (f.t + (float) i);
        g.setColour (k.accent.withAlpha (0.6f * (1.0f - fall)));
        g.fillEllipse (f.X (u), f.Y (0.3f + 0.7f * fall), f.S() * 0.015f, f.S() * 0.01f);
    }
}

void bamboo (G& g, const Frame& f)
{
    const auto& k = f.ink;
    for (int i = 0; i < 7; ++i)
    {
        const float u = 0.08f + 0.13f * (float) i + 0.03f * K::hash (f.seed, i);
        const float sway = std::sin (f.t * (0.5f + 0.2f * K::hash (f.seed, i + 5)) + (float) i) * (0.02f + 0.03f * f.flow);
        const float w = f.S() * 0.025f * (0.7f + 0.5f * K::hash (f.seed, i + 9));
        const auto base = f.P (u, 1.0f), top = f.P (u + sway, 0.0f);
        g.setColour (k.sil.interpolatedWith (k.line, 0.15f + 0.1f * (float) (i % 2)));
        g.drawLine (base.x, base.y, top.x, top.y, w);
        g.setColour (k.line.withAlpha (0.4f));
        for (int n = 1; n < 6; ++n)
        {
            const float v = (float) n / 6.0f;
            const float x = base.x + (top.x - base.x) * (1.0f - v);
            const float y = base.y + (top.y - base.y) * (1.0f - v);
            g.drawLine (x - w * 0.6f, y, x + w * 0.6f, y, 1.0f);
            if (K::hash (f.seed, i * 10 + n) > 0.5f)
            {
                juce::Path leaf;
                const float dir = K::hash (f.seed, i * 10 + n + 3) > 0.5f ? 1.0f : -1.0f;
                leaf.startNewSubPath (x, y);
                leaf.quadraticTo (x + dir * w * 3.0f, y - w * 2.0f, x + dir * w * 6.0f, y + w * 0.5f);
                leaf.quadraticTo (x + dir * w * 3.0f, y - w * 0.5f, x, y);
                g.setColour (k.accent.withAlpha (0.35f));
                g.fillPath (leaf);
            }
        }
    }
}

void fireflies (G& g, const Frame& f)
{
    const auto& k = f.ink;
    g.setColour (k.sil.interpolatedWith (k.line, 0.1f));
    for (int i = 0; i < 140; ++i)
    {
        const float x = f.box.getX() + K::hash (f.seed, i) * f.W();
        const float h = f.H() * (0.1f + 0.25f * K::hash (f.seed, i + 140));
        g.drawLine (x, f.box.getBottom(), x + std::sin (f.t + (float) i) * 2.0f, f.box.getBottom() - h, 1.0f);
    }
    for (int i = 0; i < 26; ++i)
    {
        const float u = K::hash (f.seed, i + 400) + 0.03f * std::sin (f.t * 0.5f + (float) i);
        const float v = 0.1f + 0.6f * K::hash (f.seed, i + 500) + 0.04f * std::sin (f.t * 0.7f + (float) i * 1.3f);
        const float blink = juce::jmax (0.0f, std::sin (f.t * (1.0f + K::hash (f.seed, i + 600)) + (float) i));
        K::halo (g, f.P (u, v), f.S() * 0.04f, k.warm.withAlpha (blink * (0.5f + 0.5f * f.hat)));
    }
}

void aurora (G& g, const Frame& f)
{
    const auto& k = f.ink;
    for (int band = 0; band < 3; ++band)
    {
        for (int i = 0; i < 60; ++i)
        {
            const float u = (float) i / 60.0f;
            const float y = f.Y (0.55f + 0.1f * (float) band) + std::sin (u * 6.0f + f.t * 0.3f + (float) band) * f.H() * 0.1f;
            const float h = f.H() * (0.25f + 0.25f * K::noise (f.seed + (juce::uint32) band, u * 8.0f + f.t * 0.2f));
            juce::ColourGradient curtain (k.glow.withAlpha (0.0f), 0.0f, y - h, k.glow.withAlpha (0.12f + 0.25f * f.flow + 0.1f * f.mid), 0.0f, y, false);
            g.setGradientFill (curtain);
            g.fillRect (f.X (u), y - h, f.W() / 60.0f + 0.6f, h);
        }
    }
}

void fuji (G& g, const Frame& f)
{
    const auto& k = f.ink;
    K::halo (g, f.P (0.72f, 0.25f), f.S() * 0.2f, k.accent.withAlpha (0.3f));
    g.setColour (k.accent.withAlpha (0.75f));
    g.fillEllipse (f.X (0.72f) - f.S() * 0.08f, f.Y (0.25f) - f.S() * 0.08f, f.S() * 0.16f, f.S() * 0.16f);

    juce::Path m;
    m.startNewSubPath (f.P (0.0f, 1.0f));
    m.quadraticTo (f.P (0.3f, 0.75f), f.P (0.44f, 0.32f));
    m.lineTo (f.P (0.56f, 0.32f));
    m.quadraticTo (f.P (0.7f, 0.75f), f.P (1.0f, 1.0f));
    m.closeSubPath();
    K::silhouette (g, m, f, 0.3f);

    juce::Path cap;
    cap.startNewSubPath (f.P (0.44f, 0.32f));
    cap.lineTo (f.P (0.56f, 0.32f));
    cap.lineTo (f.P (0.6f, 0.42f));
    cap.lineTo (f.P (0.55f, 0.39f));
    cap.lineTo (f.P (0.5f, 0.44f));
    cap.lineTo (f.P (0.45f, 0.39f));
    cap.lineTo (f.P (0.4f, 0.43f));
    cap.closeSubPath();
    g.setColour (k.line.withAlpha (k.light ? 0.3f : 0.6f));
    g.fillPath (cap);
    K::fog (g, f, 0.7f, 0.12f, k.haze.withAlpha (0.25f));
}

void lotus (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.P (0.5f, 0.7f);
    const float open = K::ease (f.life * 2.5f);
    for (int i = 1; i <= 3; ++i)
    {
        const float rr = f.S() * 0.15f * (float) i + std::fmod (f.t * 6.0f, 12.0f);
        g.setColour (k.line.withAlpha (0.15f));
        g.drawEllipse (c.x - rr * 1.6f, c.y - rr * 0.25f, rr * 3.2f, rr * 0.5f, 0.6f);
    }
    const int petals = 9;
    for (int i = 0; i < petals; ++i)
    {
        const float spread = ((float) i / (float) (petals - 1) - 0.5f) * (0.4f + 1.6f * open);
        juce::Path petal;
        const float len = f.S() * 0.42f * (1.0f - std::abs (spread) * 0.25f);
        petal.startNewSubPath (0.0f, 0.0f);
        petal.quadraticTo (len * 0.3f, -len * 0.5f, 0.0f, -len);
        petal.quadraticTo (-len * 0.3f, -len * 0.5f, 0.0f, 0.0f);
        petal.closeSubPath();
        petal.applyTransform (juce::AffineTransform::rotation (spread).translated (c.x, c.y));
        g.setColour (k.accent.withAlpha (0.18f));
        g.fillPath (petal);
        K::glowLine (g, petal, k.line.withAlpha (0.8f), k.glow, 0.8f, 0.3f + 0.4f * f.mid, k.light);
    }
}

void loneTree (G& g, const Frame& f)
{
    const auto& k = f.ink;
    juce::Path hill;
    hill.startNewSubPath (f.P (0.0f, 1.0f));
    hill.quadraticTo (f.P (0.5f, 0.72f), f.P (1.0f, 1.0f));
    hill.closeSubPath();
    K::silhouette (g, hill, f, 0.2f);

    juce::Path tree;
    struct B { Pt a; float ang, len; int depth; };
    std::vector<B> stack { { f.P (0.5f, 0.79f), -pi * 0.5f, f.H() * 0.22f, 0 } };
    const float sway = std::sin (f.t * 0.6f) * 0.04f * (0.5f + f.flow);
    while (! stack.empty())
    {
        const auto b = stack.back();
        stack.pop_back();
        const Pt e (b.a.x + std::cos (b.ang + sway * (float) b.depth) * b.len, b.a.y + std::sin (b.ang + sway * (float) b.depth) * b.len);
        tree.startNewSubPath (b.a);
        tree.lineTo (e);
        if (b.depth < 6)
        {
            const float spread = 0.35f + 0.25f * K::hash (f.seed, b.depth * 7 + (int) stack.size());
            stack.push_back ({ e, b.ang - spread, b.len * 0.72f, b.depth + 1 });
            stack.push_back ({ e, b.ang + spread, b.len * 0.7f, b.depth + 1 });
        }
    }
    g.setColour (k.light ? k.sil : k.line.withAlpha (0.75f));
    g.strokePath (tree, K::pen (1.0f));

    for (int i = 0; i < 8; ++i)
    {
        const float fall = std::fmod (f.t * 0.06f + K::hash (f.seed, i + 100), 1.0f);
        g.setColour (k.accent.withAlpha (0.6f * (1.0f - fall)));
        g.fillEllipse (f.X (0.35f + 0.3f * K::hash (f.seed, i + 120) + 0.1f * fall), f.Y (0.35f + 0.45f * fall), 3.0f, 2.0f);
    }
}
} // namespace

void addNature (std::vector<Spec>& v)
{
    const int hero = flagHero, both = flagHero | flagCompanion, comp = flagCompanion;
    //            name                    category            fn          zone            size  aspect  life           quiet bass  melod energ   flags
    v.push_back ({ "Whale passing",        Category::Nature, whale,      Zone::BandMid,  0.42f, 1.0f, 18.0f, 28.0f, { 0.8f, 0.8f, 0.7f, 0.1f }, hero });
    v.push_back ({ "Two koi",              Category::Nature, koi,        Zone::Centre,   0.6f,  1.4f, 12.0f, 22.0f, { 0.7f, 0.3f, 0.9f, 0.2f }, hero });
    v.push_back ({ "Jellyfish",            Category::Nature, jellyfish,  Zone::Anywhere, 0.55f, 0.7f, 12.0f, 20.0f, { 0.8f, 0.5f, 0.7f, 0.1f }, both });
    v.push_back ({ "Flock of birds",       Category::Nature, flock,      Zone::BandSky,  0.3f,  1.0f, 12.0f, 18.0f, { 0.3f, 0.2f, 0.7f, 0.8f }, comp });
    v.push_back ({ "Geese in formation",   Category::Nature, geese,      Zone::BandSky,  0.26f, 1.0f, 12.0f, 18.0f, { 0.5f, 0.2f, 0.8f, 0.4f }, comp });
    v.push_back ({ "Ravens",               Category::Nature, ravens,     Zone::BandSky,  0.22f, 1.0f, 10.0f, 16.0f, { 0.3f, 0.6f, 0.4f, 0.6f }, comp });
    v.push_back ({ "Rain cloud",           Category::Nature, rainCloud,  Zone::Sky,      0.5f,  1.5f, 12.0f, 22.0f, { 0.8f, 0.3f, 0.6f, 0.2f }, both });
    v.push_back ({ "Distant storm",        Category::Nature, storm,      Zone::Sky,      0.5f,  2.2f, 10.0f, 18.0f, { 0.2f, 0.8f, 0.3f, 0.9f }, hero });
    v.push_back ({ "High cirrus",          Category::Nature, cirrus,     Zone::Sky,      0.4f,  2.6f, 16.0f, 30.0f, { 0.9f, 0.2f, 0.7f, 0.0f }, comp });
    v.push_back ({ "Snow on the peaks",    Category::Nature, snowPeaks,  Zone::Ground,   0.62f, 2.4f, 16.0f, 28.0f, { 0.8f, 0.4f, 0.5f, 0.2f }, hero });
    v.push_back ({ "Mountains in fog",     Category::Nature, fogRanges,  Zone::Ground,   0.7f,  2.6f, 18.0f, 32.0f, { 1.0f, 0.3f, 0.6f, 0.0f }, hero });
    v.push_back ({ "Pine forest",          Category::Nature, pineForest, Zone::Ground,   0.5f,  3.0f, 14.0f, 26.0f, { 0.6f, 0.7f, 0.4f, 0.3f }, hero });
    v.push_back ({ "Moon on the sea",      Category::Nature, moonSea,    Zone::Ground,   0.75f, 1.8f, 16.0f, 30.0f, { 1.0f, 0.2f, 0.7f, 0.0f }, hero });
    v.push_back ({ "Night waves",          Category::Nature, waves,      Zone::Ground,   0.4f,  3.0f, 14.0f, 24.0f, { 0.6f, 0.7f, 0.5f, 0.4f }, hero });
    v.push_back ({ "Cherry blossom",       Category::Nature, sakura,     Zone::Anywhere, 0.6f,  1.4f, 14.0f, 24.0f, { 0.7f, 0.1f, 0.9f, 0.1f }, hero });
    v.push_back ({ "Bamboo grove",         Category::Nature, bamboo,     Zone::Ground,   0.85f, 1.0f, 14.0f, 24.0f, { 0.7f, 0.3f, 0.8f, 0.1f }, hero });
    v.push_back ({ "Fireflies",            Category::Nature, fireflies,  Zone::Ground,   0.45f, 2.6f, 14.0f, 24.0f, { 0.9f, 0.1f, 0.6f, 0.1f }, both });
    v.push_back ({ "Aurora",               Category::Nature, aurora,     Zone::Sky,      0.55f, 2.8f, 16.0f, 30.0f, { 0.8f, 0.3f, 0.9f, 0.2f }, both });
    v.push_back ({ "Mountain and red sun", Category::Nature, fuji,       Zone::Ground,   0.62f, 2.0f, 16.0f, 28.0f, { 0.8f, 0.3f, 0.6f, 0.1f }, hero });
    v.push_back ({ "Lotus opening",        Category::Nature, lotus,      Zone::Centre,   0.5f,  1.3f, 12.0f, 20.0f, { 0.8f, 0.1f, 0.9f, 0.1f }, hero });
    v.push_back ({ "Lone tree on a hill",  Category::Nature, loneTree,   Zone::Ground,   0.62f, 1.5f, 14.0f, 26.0f, { 0.9f, 0.2f, 0.7f, 0.0f }, hero });
}
}

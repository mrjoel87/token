// =================================================================================================
//  SURREAL OBJECTS - dreamlike and occasionally unsettling
//  (the number transmission and the sigils are invented - no hidden meaning)
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

juce::Path almond (Pt c, float w, float h)
{
    juce::Path p;
    p.startNewSubPath (c.x - w, c.y);
    p.quadraticTo (c.x, c.y - h, c.x + w, c.y);
    p.quadraticTo (c.x, c.y + h, c.x - w, c.y);
    p.closeSubPath();
    return p;
}

void drawEye (G& g, const Frame& f, Pt c, float w, float open, float look)
{
    const auto& k = f.ink;
    const float h = w * 0.42f * open;
    if (h < 0.4f)
    {
        g.setColour (k.line.withAlpha (0.6f));
        g.drawLine (c.x - w, c.y, c.x + w, c.y, 0.8f);
        return;
    }
    const auto eye = almond (c, w, h);
    g.setColour (k.light ? k.bg : k.sil);
    g.fillPath (eye);
    {
        G::ScopedSaveState state (g);
        g.reduceClipRegion (eye);
        const float ir = w * 0.38f;
        const Pt ic (c.x + look * w * 0.3f, c.y);
        juce::ColourGradient iris (k.accent, ic.x, ic.y, k.glow.withAlpha (0.2f), ic.x + ir, ic.y, true);
        g.setGradientFill (iris);
        g.fillEllipse (ic.x - ir, ic.y - ir, ir * 2.0f, ir * 2.0f);
        g.setColour (k.light ? k.sil : juce::Colours::black);
        const float pr = ir * (0.42f - 0.12f * f.kick);
        g.fillEllipse (ic.x - pr, ic.y - pr, pr * 2.0f, pr * 2.0f);
        g.setColour (k.line.withAlpha (0.8f));
        g.fillEllipse (ic.x - ir * 0.45f, ic.y - ir * 0.5f, ir * 0.2f, ir * 0.2f);
    }
    K::glowLine (g, eye, k.line, k.glow, 0.9f, 0.5f, k.light);
}

// ------------------------------------------------------------------------------------------------
void floatingIsland (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float bob = std::sin (f.t * 0.5f) * f.H() * 0.03f;
    juce::Path rock;
    rock.startNewSubPath (f.X (0.15f), f.Y (0.45f) + bob);
    rock.lineTo (f.X (0.85f), f.Y (0.45f) + bob);
    rock.quadraticTo (f.X (0.65f), f.Y (0.65f) + bob, f.X (0.52f), f.Y (0.95f) + bob);
    rock.quadraticTo (f.X (0.35f), f.Y (0.65f) + bob, f.X (0.15f), f.Y (0.45f) + bob);
    rock.closeSubPath();
    for (int i = 0; i < 4; ++i)
    {
        const float u = 0.3f + 0.12f * (float) i;
        const float h = i == 2 ? 0.1f : 0.2f;
        rock.addRectangle (f.X (u) - f.W() * 0.012f, f.Y (0.45f - h) + bob, f.W() * 0.024f, f.H() * h);
    }
    rock.addRectangle (f.X (0.28f), f.Y (0.24f) + bob, f.W() * 0.3f, f.H() * 0.025f);
    K::silhouette (g, rock, f, 0.3f + 0.4f * f.bass);
    g.setColour (k.line.withAlpha (0.3f + 0.2f * f.high));
    for (int i = 0; i < 3; ++i)
    {
        const float x = f.X (0.8f + 0.02f * (float) i);
        g.drawLine (x, f.Y (0.46f) + bob, x + 1.0f, f.box.getBottom(), 0.7f);
    }
}

void seaDoor (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float water = 0.72f;
    g.setColour (k.line.withAlpha (0.3f));
    g.drawLine (f.box.getX(), f.Y (water), f.box.getRight(), f.Y (water), 0.7f);
    const auto door = Rc (f.X (0.42f) - (f.flip ? f.W() * 0.16f : 0.0f), f.Y (0.25f), f.W() * 0.16f, f.H() * 0.47f);
    const float light = 0.6f + 0.4f * f.mid;
    K::halo (g, door.getCentre(), door.getHeight(), k.warm.withAlpha (0.25f * light));
    g.setColour (k.warm.interpolatedWith (k.line, 0.4f).withAlpha (0.85f));
    g.fillRect (door);
    g.setColour (k.sil);
    g.drawRect (door.expanded (2.0f), 2.5f);
    K::reflection (g, door.getCentreX(), f.Y (water) + 2.0f, door.getWidth() * 1.2f, f.H() * 0.25f, k.warm.withAlpha (0.4f), f.t, f.seed);
}

void eyeInGeometry (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.S() * 0.42f;
    juce::Path star;
    star.addPath (K::polygon (c, 3, r, f.t * 0.05f));
    star.addPath (K::polygon (c, 3, r, f.t * 0.05f + pi));
    star.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    K::glowLine (g, star, k.line.withAlpha (0.7f), k.glow, 0.8f, 0.3f + 0.5f * f.bass, k.light);
    const float open = K::ease ((f.life - 0.3f) / 0.35f) * (std::fmod (f.t, 6.0f) < 0.15f ? 0.1f : 1.0f);
    drawEye (g, f, c, r * 0.32f, open, std::sin (f.t * 0.6f));
}

void skyLanterns (G& g, const Frame& f)
{
    const auto& k = f.ink;
    for (int i = 0; i < 16; ++i)
    {
        const float speed = 0.03f + 0.04f * K::hash (f.seed, i);
        const float v = 1.0f - std::fmod (K::hash (f.seed, i + 20) + f.t * speed, 1.1f);
        const float u = K::hash (f.seed, i + 40) + 0.02f * std::sin (f.t * 0.5f + (float) i);
        const float s = f.S() * (0.03f + 0.03f * K::hash (f.seed, i + 60)) * (0.6f + 0.4f * v);
        const auto p = f.P (u, v);
        K::halo (g, p, s * 3.0f, k.warm.withAlpha (0.35f));
        g.setColour (k.warm.withAlpha (0.85f));
        g.fillRoundedRectangle (p.x - s * 0.5f, p.y - s * 0.7f, s, s * 1.3f, s * 0.2f);
    }
}

void wireCube (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float s = f.S() * 0.28f * (1.0f + 0.06f * f.kick);
    const float ay = f.t * (0.3f + 0.4f * f.energy), ax = f.t * 0.21f;
    Pt v[8];
    for (int i = 0; i < 8; ++i)
    {
        float x = (i & 1) ? 1.0f : -1.0f, y = (i & 2) ? 1.0f : -1.0f, z = (i & 4) ? 1.0f : -1.0f;
        const float x1 = x * std::cos (ay) + z * std::sin (ay), z1 = -x * std::sin (ay) + z * std::cos (ay);
        const float y1 = y * std::cos (ax) - z1 * std::sin (ax), z2 = y * std::sin (ax) + z1 * std::cos (ax);
        const float persp = 3.0f / (3.0f + z2);
        v[i] = { c.x + x1 * s * persp, c.y + y1 * s * persp };
    }
    juce::Path p;
    const int edges[12][2] = { { 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 }, { 4, 5 }, { 5, 7 }, { 7, 6 }, { 6, 4 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
    for (const auto& e : edges)
    {
        p.startNewSubPath (v[e[0]]);
        p.lineTo (v[e[1]]);
    }
    K::glowLine (g, p, k.line, k.glow, 1.0f, 0.4f + 0.6f * f.bass, k.light);
}

void impossibleStairs (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float s = f.S() * 0.36f;
    juce::Path p;
    const Pt corners[4] = { { c.x - s, c.y }, { c.x, c.y - s * 0.55f }, { c.x + s, c.y }, { c.x, c.y + s * 0.55f } };
    for (int side = 0; side < 4; ++side)
    {
        const auto a = corners[side], b = corners[(side + 1) % 4];
        const int steps = 5;
        Pt cur = a;
        p.startNewSubPath (cur);
        for (int i = 1; i <= steps; ++i)
        {
            const Pt next (a.x + (b.x - a.x) * (float) i / (float) steps, a.y + (b.y - a.y) * (float) i / (float) steps);
            p.lineTo (cur.x, cur.y - s * 0.06f);
            p.lineTo (next.x, next.y - s * 0.06f);
            p.lineTo (next);
            cur = next;
        }
    }
    K::glowLine (g, p, k.line.withAlpha (0.85f), k.glow, 0.9f, 0.4f + 0.4f * f.mid, k.light);
    const float pos = std::fmod (f.t * 0.12f, 1.0f) * 4.0f;
    const int side = (int) pos;
    const float fr = pos - (float) side;
    const auto a = corners[side % 4], b = corners[(side + 1) % 4];
    K::person (g, a.x + (b.x - a.x) * fr, a.y + (b.y - a.y) * fr - s * 0.06f, juce::jmax (1.0f, f.px * 0.8f),
               f.ink.light ? f.ink.sil : f.ink.line, K::Pose::Walk, f.t * 3.0f, 1.0f);
}

void moonOnString (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float sway = std::sin (f.t * 0.7f) * 0.06f;
    const Pt top (f.X (0.5f), f.box.getY());
    const float len = f.H() * 0.6f;
    const Pt m (top.x + std::sin (sway) * len, top.y + std::cos (sway) * len);
    g.setColour (k.line.withAlpha (0.4f));
    g.drawLine (top.x, top.y, m.x, m.y, 0.6f);
    K::moon (g, m, f.S() * 0.26f, k.line.withAlpha (k.light ? 0.3f : 0.85f), 1.0f + f.bass);
}

void handlessClock (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.S() * 0.42f;
    juce::Path face;
    face.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    K::silhouette (g, face, f, 0.3f);
    const int beatIndex = (int) std::floor (f.beat * 12.0f);
    for (int i = 0; i < 12; ++i)
    {
        const float a = (float) i / 12.0f * twoPi;
        const bool lit = i == beatIndex;
        g.setColour (lit ? k.accent : k.line.withAlpha (0.5f));
        g.drawLine (c.x + std::sin (a) * r * 0.82f, c.y - std::cos (a) * r * 0.82f, c.x + std::sin (a) * r * 0.95f, c.y - std::cos (a) * r * 0.95f, lit ? 2.0f : 1.0f);
    }
    K::halo (g, c, r * 0.08f, k.accent.withAlpha (0.6f));
}

void stoneHand (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float water = 0.85f;
    const float rise = K::ease (f.life * 3.0f);
    const float base = f.Y (water) + (1.0f - rise) * f.H() * 0.5f;
    juce::Path hand;
    hand.startNewSubPath (f.X (0.4f), base);
    hand.lineTo (f.X (0.42f), base - f.H() * 0.35f);
    const float fingers[] = { 0.42f, 0.47f, 0.52f, 0.57f };
    const float heights[] = { 0.55f, 0.62f, 0.6f, 0.52f };
    for (int i = 0; i < 4; ++i)
    {
        hand.lineTo (f.X (fingers[i]), base - f.H() * heights[i]);
        hand.lineTo (f.X (fingers[i] + 0.04f), base - f.H() * heights[i]);
        hand.lineTo (f.X (fingers[i] + 0.04f), base - f.H() * 0.38f);
    }
    hand.lineTo (f.X (0.63f), base - f.H() * 0.3f);
    hand.lineTo (f.X (0.68f), base - f.H() * 0.4f);
    hand.lineTo (f.X (0.62f), base);
    hand.closeSubPath();
    {
        G::ScopedSaveState state (g);
        g.reduceClipRegion (Rc (f.box.getX(), f.box.getY(), f.W(), f.Y (water) - f.box.getY()).toNearestInt());
        K::silhouette (g, hand, f, 0.3f + 0.4f * f.bass);
    }
    g.setColour (k.line.withAlpha (0.3f));
    g.drawLine (f.box.getX(), f.Y (water), f.box.getRight(), f.Y (water), 0.7f);
    for (int i = 1; i <= 3; ++i)
    {
        const float rr = f.W() * 0.06f * (float) i + std::fmod (f.t * 5.0f, 10.0f);
        g.setColour (k.line.withAlpha (0.15f));
        g.drawEllipse (f.X (0.53f) - rr, f.Y (water) - rr * 0.15f, rr * 2.0f, rr * 0.3f, 0.6f);
    }
}

void hourglass (G& g, const Frame& f)
{
    const auto& k = f.ink;
    juce::Path glass;
    glass.startNewSubPath (f.P (0.15f, 0.05f));
    glass.lineTo (f.P (0.85f, 0.05f));
    glass.quadraticTo (f.P (0.85f, 0.35f), f.P (0.53f, 0.5f));
    glass.quadraticTo (f.P (0.85f, 0.65f), f.P (0.85f, 0.95f));
    glass.lineTo (f.P (0.15f, 0.95f));
    glass.quadraticTo (f.P (0.15f, 0.65f), f.P (0.47f, 0.5f));
    glass.quadraticTo (f.P (0.15f, 0.35f), f.P (0.15f, 0.05f));
    glass.closeSubPath();
    const float sand = std::fmod (f.t * 0.04f, 1.0f);
    {
        G::ScopedSaveState state (g);
        g.reduceClipRegion (glass);
        g.setColour (k.accent.withAlpha (0.6f));
        g.fillRect (f.box.getX(), f.Y (0.05f + 0.4f * sand), f.W(), f.H() * 0.45f * (1.0f - sand));
        g.fillRect (f.box.getX(), f.Y (0.95f - 0.4f * sand), f.W(), f.H() * 0.45f * sand);
    }
    g.setColour (k.accent.withAlpha (0.8f));
    g.drawLine (f.X (0.5f), f.Y (0.5f), f.X (0.5f), f.Y (0.95f - 0.4f * sand), 1.0f);
    K::glowLine (g, glass, k.line, k.glow, 1.0f, 0.4f + 0.4f * f.mid, k.light);
}

void cloudStair (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.P (0.5f, 0.5f);
    juce::Path p;
    const int steps = 26;
    for (int i = 0; i < steps; ++i)
    {
        const float a = (float) i * 0.5f + f.t * 0.1f;
        const float v = 0.95f - (float) i / (float) steps * 0.8f;
        const float rx = f.W() * 0.3f * (1.0f - (float) i / (float) steps * 0.4f);
        const float x = c.x + std::sin (a) * rx;
        const float depth = std::cos (a);
        const float w = f.W() * 0.05f * (0.7f + 0.3f * depth);
        if (depth > -0.2f)
        {
            p.startNewSubPath (x - w, f.Y (v));
            p.lineTo (x + w, f.Y (v));
        }
    }
    K::glowLine (g, p, k.line.withAlpha (0.8f), k.glow, 1.4f, 0.4f, k.light);
    K::cloud (g, f.X (0.5f), f.Y (0.15f), f.W() * 0.7f, f.H() * 0.18f, k.sil.withAlpha (0.8f), k.haze.withAlpha (0.2f), f.seed);
}

void portal (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.S() * 0.36f * (1.0f + 0.06f * f.kick);
    const float spin = f.t * (0.2f + 0.8f * f.energy);
    juce::ColourGradient core (k.light ? k.sil : juce::Colours::black, c.x, c.y, k.glow.withAlpha (0.0f), c.x + r * 1.2f, c.y, true);
    core.addColour (0.75, k.glow.withAlpha (0.35f));
    g.setGradientFill (core);
    g.fillEllipse (c.x - r * 1.2f, c.y - r * 1.2f, r * 2.4f, r * 2.4f);
    juce::Path rings;
    rings.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    rings.addPath (K::polygon (c, 6, r * 0.82f, spin));
    rings.addPath (K::polygon (c, 3, r * 0.7f, -spin * 0.7f));
    rings.addPath (K::polygon (c, 3, r * 0.7f, -spin * 0.7f + pi));
    K::glowLine (g, rings, k.line, k.glow, 1.0f, 0.5f + 0.5f * f.bass, k.light);
    g.setColour (k.accent.withAlpha (0.6f));
    for (int i = 0; i < 24; ++i)
    {
        const float a = spin * 0.3f + (float) i / 24.0f * twoPi;
        const float r1 = r * (i % 6 == 0 ? 1.22f : 1.12f);
        g.drawLine (c.x + std::sin (a) * r * 1.05f, c.y - std::cos (a) * r * 1.05f, c.x + std::sin (a) * r1, c.y - std::cos (a) * r1, 0.8f);
    }
}

void manyEyes (G& g, const Frame& f)
{
    for (int i = 0; i < 9; ++i)
    {
        const float delay = K::hash (f.seed, i + 60) * 4.0f;
        const float open = K::ease ((f.t - delay) * 0.8f) * (std::fmod (f.t + (float) i, 4.5f) < 0.15f ? 0.1f : 1.0f);
        drawEye (g, f, f.P (0.1f + 0.8f * K::hash (f.seed, i), 0.15f + 0.7f * K::hash (f.seed, i + 30)),
                 f.S() * (0.05f + 0.04f * K::hash (f.seed, i + 90)), open, std::sin (f.t * 0.7f + (float) i));
    }
}

void numberStation (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const int groups = 3, perGroup = 5, rows = 2;
    const int total = rows * groups * perGroup;
    const int shown = juce::jmin (total, (int) (f.t * 6.0f));
    const float cw = f.W() / 22.0f, ch = f.H() * 0.3f;
    const float th = juce::jmax (1.0f, f.S() * 0.03f);
    for (int d = 0; d < shown; ++d)
    {
        const int row = d / (groups * perGroup), col = d % (groups * perGroup);
        const float x = f.box.getX() + cw + (float) (col / perGroup) * cw * 7.0f + (float) (col % perGroup) * cw * 1.3f;
        const float y = f.box.getY() + f.H() * 0.12f + (float) row * ch * 1.5f;
        const bool fresh = d >= shown - 2;
        K::sevenSeg (g, (int) (K::hash (f.seed, d) * 10.0f), Rc (x, y, cw * 0.8f, ch), fresh ? k.line : k.accent.withAlpha (0.7f), th);
    }
}

void staticFace (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float s = f.S() * 0.42f;
    const int frame = (int) (f.t * 20.0f);
    for (int i = 0; i < 260; ++i)
    {
        const float a = K::hash (f.seed + (juce::uint32) frame, i * 2) * twoPi;
        const float r = std::sqrt (K::hash (f.seed + (juce::uint32) frame, i * 2 + 1)) * s;
        g.setColour (k.line.withAlpha (0.1f + 0.4f * K::hash (f.seed + 77u, i + frame)));
        g.fillRect (c.x + std::sin (a) * r * 0.8f, c.y + std::cos (a) * r, 1.5f, 1.5f);
    }
    const float breathe = 1.0f + 0.04f * std::sin (f.t * 0.9f);
    K::blob (g, c, s * 0.6f * breathe, s * 0.82f * breathe, k.line.withAlpha (0.25f + 0.2f * f.mid));
    const float open = 0.6f + 0.4f * std::sin (f.t * 0.6f);
    const auto dark = k.light ? k.sil.withAlpha (0.9f) : juce::Colours::black.withAlpha (0.9f);
    K::blob (g, { c.x - s * 0.22f, c.y - s * 0.12f }, s * 0.13f, s * 0.1f * open, dark);
    K::blob (g, { c.x + s * 0.22f, c.y - s * 0.12f }, s * 0.13f, s * 0.1f * open, dark);
    K::blob (g, { c.x, c.y + s * 0.38f }, s * 0.09f, s * (0.04f + 0.1f * f.mid), dark);
}

void corridor (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto vp = f.P (0.5f, 0.45f);
    g.setColour (k.line.withAlpha (0.3f));
    g.drawLine (f.box.getX(), f.box.getY(), vp.x - f.W() * 0.06f, vp.y - f.H() * 0.1f, 0.8f);
    g.drawLine (f.box.getRight(), f.box.getY(), vp.x + f.W() * 0.06f, vp.y - f.H() * 0.1f, 0.8f);
    g.drawLine (f.box.getX(), f.box.getBottom(), vp.x - f.W() * 0.06f, vp.y + f.H() * 0.1f, 0.8f);
    g.drawLine (f.box.getRight(), f.box.getBottom(), vp.x + f.W() * 0.06f, vp.y + f.H() * 0.1f, 0.8f);
    for (int i = 1; i < 6; ++i)
    {
        const float s = 1.0f / (1.0f + (float) i * 0.8f);
        g.setColour (k.line.withAlpha (0.12f + 0.04f * (float) i));
        g.drawRect (vp.x - f.W() * 0.5f * s, vp.y - f.H() * 0.55f * s, f.W() * s, f.H() * 1.1f * s, 0.6f);
    }
    K::halo (g, vp, f.H() * 0.15f, k.line.withAlpha (0.5f));
    const float flicker = std::fmod (f.t, 3.7f) < 0.08f ? 0.3f : 1.0f;
    K::person (g, vp.x, vp.y + f.H() * 0.1f, juce::jmax (1.0f, f.px * 1.1f), (k.light ? k.sil : juce::Colours::black).withAlpha (flicker), K::Pose::Stand, 0.0f, 1.0f);
}

void tinyWorld (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.P (0.5f, 0.6f);
    const float r = f.S() * 0.3f;
    juce::Path planet;
    planet.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    K::halo (g, c, r * 1.8f, k.haze.withAlpha (0.2f));
    K::silhouette (g, planet, f, 0.4f);

    const float spin = f.t * 0.08f;
    G::ScopedSaveState state (g);
    g.addTransform (juce::AffineTransform::rotation (std::sin (spin) * 0.4f, c.x, c.y));
    const float px = f.px;
    const auto col = k.light ? k.sil : k.line.withAlpha (0.8f);
    K::pixelRect (g, c.x - px * 4.0f, c.y - r - px * 5.0f, px * 6.0f, px * 5.0f, col);
    juce::Path roof;
    roof.addTriangle (c.x - px * 5.0f, c.y - r - px * 5.0f, c.x + px * 3.0f, c.y - r - px * 5.0f, c.x - px, c.y - r - px * 8.0f);
    g.setColour (col);
    g.fillPath (roof);
    K::pixelRect (g, c.x - px * 2.0f, c.y - r - px * 3.0f, px * 1.5f, px * 1.5f, k.warm);
    g.setColour (col);
    g.drawLine (c.x + px * 6.0f, c.y - r, c.x + px * 6.0f, c.y - r - px * 7.0f, 1.0f);
    K::blob (g, { c.x + px * 6.0f, c.y - r - px * 8.0f }, px * 3.0f, px * 2.5f, k.line.withAlpha (0.6f));
}
} // namespace

void addSurreal (std::vector<Spec>& v)
{
    const int hero = flagHero, sym = flagHero | flagNoFlip;
    //            name                        category            fn               zone            size  aspect  life           quiet bass  melod energ   flags
    v.push_back ({ "Floating island",          Category::Surreal, floatingIsland,  Zone::Anywhere, 0.6f,  1.3f, 14.0f, 26.0f, { 0.7f, 0.4f, 0.7f, 0.2f }, hero });
    v.push_back ({ "Door in the sea",          Category::Surreal, seaDoor,         Zone::Ground,   0.6f,  1.3f, 14.0f, 24.0f, { 0.8f, 0.3f, 0.6f, 0.1f }, hero });
    v.push_back ({ "Eye in the geometry",      Category::Surreal, eyeInGeometry,   Zone::Centre,   0.68f, 1.0f, 12.0f, 20.0f, { 0.3f, 0.8f, 0.3f, 0.8f }, sym });
    v.push_back ({ "Sky lanterns",             Category::Surreal, skyLanterns,     Zone::Anywhere, 0.75f, 1.5f, 16.0f, 28.0f, { 0.9f, 0.1f, 0.9f, 0.1f }, hero | flagCompanion });
    v.push_back ({ "Turning cube",             Category::Surreal, wireCube,        Zone::Centre,   0.5f,  1.0f, 10.0f, 18.0f, { 0.2f, 0.6f, 0.3f, 0.9f }, sym });
    v.push_back ({ "Endless stairs",           Category::Surreal, impossibleStairs,Zone::Centre,   0.62f, 1.4f, 14.0f, 24.0f, { 0.5f, 0.4f, 0.6f, 0.5f }, hero });
    v.push_back ({ "Moon on a string",         Category::Surreal, moonOnString,    Zone::Sky,      0.6f,  0.7f, 14.0f, 24.0f, { 0.9f, 0.3f, 0.7f, 0.0f }, hero });
    v.push_back ({ "Clock without hands",      Category::Surreal, handlessClock,   Zone::Anywhere, 0.45f, 1.0f, 10.0f, 18.0f, { 0.4f, 0.4f, 0.4f, 0.7f }, sym });
    v.push_back ({ "Hand from the sea",        Category::Surreal, stoneHand,       Zone::Ground,   0.62f, 1.0f, 12.0f, 20.0f, { 0.4f, 0.8f, 0.3f, 0.4f }, hero | flagRare });
    v.push_back ({ "Hourglass",                Category::Surreal, hourglass,       Zone::Centre,   0.55f, 0.6f, 12.0f, 22.0f, { 0.8f, 0.3f, 0.6f, 0.1f }, sym });
    v.push_back ({ "Staircase into cloud",     Category::Surreal, cloudStair,      Zone::Centre,   0.75f, 0.8f, 14.0f, 24.0f, { 0.7f, 0.3f, 0.8f, 0.2f }, hero });
    v.push_back ({ "Portal",                   Category::Surreal, portal,          Zone::Centre,   0.62f, 1.0f, 10.0f, 18.0f, { 0.1f, 0.9f, 0.3f, 1.0f }, sym });
    v.push_back ({ "Many eyes",                Category::Surreal, manyEyes,        Zone::Full,     1.0f,  1.0f, 8.0f,  14.0f, { 0.2f, 0.8f, 0.2f, 0.8f }, sym | flagRare });
    v.push_back ({ "Number transmission",      Category::Surreal, numberStation,   Zone::Centre,   0.35f, 2.6f, 9.0f,  14.0f, { 0.3f, 0.5f, 0.3f, 0.7f }, sym | flagRare });
    v.push_back ({ "Face in the static",       Category::Surreal, staticFace,      Zone::Centre,   0.65f, 0.9f, 8.0f,  14.0f, { 0.5f, 0.9f, 0.3f, 0.4f }, sym | flagRare });
    v.push_back ({ "End of the corridor",      Category::Surreal, corridor,        Zone::Centre,   0.75f, 1.4f, 10.0f, 18.0f, { 0.5f, 0.8f, 0.4f, 0.3f }, sym });
    v.push_back ({ "A tiny world",             Category::Surreal, tinyWorld,       Zone::Anywhere, 0.5f,  1.0f, 14.0f, 24.0f, { 0.8f, 0.2f, 0.8f, 0.1f }, hero });
}
}

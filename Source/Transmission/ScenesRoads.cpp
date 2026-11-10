// =================================================================================================
//  ROADS, CITIES & STRANGE JOURNEYS
//  Anything that "passes" (streetlights, dashes, poles, sleepers, tunnel rings) is driven by the
//  beat position, so it moves in time with Ableton's tempo when the transport is running.
// =================================================================================================
#include "SceneKit.h"
#include <cmath>

namespace token::tx
{
namespace
{
using namespace kit;
using G = juce::Graphics;
using R = juce::Rectangle<float>;
using P = SceneParams;

float frac (float x) noexcept { return x - std::floor (x); }

/** Perspective depth for an object approaching once per beat: 0 = horizon, 1 = at the viewer. */
float approach (float beat, float offset) noexcept
{
    const float d = frac (beat + offset);
    return d * d * d;
}

void nightSky (G& g, const Frame& f, const P& p, float horizon)
{
    const auto& k = p.ink;
    sky (g, f, k.deep.withAlpha (0.0f), k.haze.withAlpha (0.45f), k.deep.withAlpha (0.6f), horizon);
}

void motorway (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const float horizon = 0.42f;
    nightSky (g, f, p, horizon);
    const juce::Point<float> vp (f.X (0.5f), f.Y (horizon));
    glow (g, vp, f.H() * 0.25f, k.haze.withAlpha (0.5f));

    juce::Path road;
    road.startNewSubPath (vp.x - 2.0f, vp.y);
    road.lineTo (vp.x + 2.0f, vp.y);
    road.lineTo (f.X (1.1f), f.Y (1.0f));
    road.lineTo (f.X (-0.1f), f.Y (1.0f));
    road.closeSubPath();
    g.setColour (k.deep.withAlpha (p.variant == 1 ? 0.5f : 0.75f));
    g.fillPath (road);

    // lane dashes and lamp posts approach in time with the beat
    for (int i = 0; i < 8; ++i)
    {
        const float d = approach (p.beat * 0.5f, (float) i / 8.0f);
        const float y = vp.y + (f.Y (1.0f) - vp.y) * d;
        const float len = (f.Y (1.0f) - vp.y) * d * 0.15f;
        g.setColour (k.light.withAlpha (0.2f + 0.6f * d));
        for (int lane = -1; lane <= 1; lane += 2)
        {
            const float x = vp.x + (float) lane * f.W() * 0.2f * d;
            g.drawLine (x, y, x + (float) lane * f.W() * 0.03f * d, y + len, 1.0f + 3.0f * d);
        }

        for (int side = -1; side <= 1; side += 2)
        {
            const float x = vp.x + (float) side * f.W() * 0.75f * d;
            const float h = f.H() * 0.7f * d;
            g.setColour (k.sil.withAlpha (0.4f + 0.6f * d));
            g.drawLine (x, y, x, y - h, 1.0f + 2.0f * d);
            glow (g, { x - (float) side * h * 0.15f, y - h }, f.H() * 0.12f * d + 2.0f, k.warm.withAlpha (0.4f + 0.4f * d));
            if (p.variant == 1)
                reflection (g, x - (float) side * h * 0.15f, y + 2.0f, h * 0.1f, h * 0.5f, k.warm.withAlpha (0.25f * d), p.t, p.seed + (juce::uint32) i);
        }
    }

    if (p.variant == 2)
    {
        // tail-lights of a convoy ahead
        for (int c = 0; c < 4; ++c)
        {
            const float d = 0.05f + 0.06f * (float) c + 0.01f * std::sin (p.t * 0.4f + (float) c);
            const float y = vp.y + (f.Y (1.0f) - vp.y) * d;
            const float x = vp.x + f.W() * 0.06f * d * ((c % 2) == 0 ? -1.0f : 1.0f) * 3.0f;
            for (int s = -1; s <= 1; s += 2)
                glow (g, { x + (float) s * f.W() * 0.1f * d, y }, f.H() * 0.02f + 1.5f, k.warm.interpolatedWith (k.accent, 0.2f));
        }
    }

    if (p.variant == 1)
        rain (g, f, p.seed, p.t, 140, k.light.withAlpha (0.3f), -0.3f);
}

void overheadLights (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    sky (g, f, k.deep.withAlpha (0.4f), k.haze.withAlpha (0.3f), k.deep.withAlpha (0.7f), 0.5f);
    const juce::Point<float> vp (f.X (0.5f), f.Y (0.6f));

    for (int i = 0; i < 10; ++i)
    {
        const float d = approach (p.beat, (float) i / 10.0f);
        const float y = vp.y - (vp.y - f.Y (-0.2f)) * d;
        const float w = f.W() * (0.02f + 0.6f * d);
        glow (g, { vp.x, y }, w * 0.6f, k.warm.withAlpha (0.25f + 0.5f * d));
        g.setColour (k.light.withAlpha (0.3f + 0.7f * d));
        g.fillRoundedRectangle (vp.x - w * 0.25f, y - 2.0f - 6.0f * d, w * 0.5f, 4.0f + 12.0f * d, 3.0f);
    }
}

void forestRoad (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const float horizon = 0.48f;
    nightSky (g, f, p, horizon);
    const juce::Point<float> vp (f.X (0.5f), f.Y (horizon));

    juce::Path cone;
    cone.startNewSubPath (f.X (0.35f), f.Y (1.0f));
    cone.lineTo (vp.x - 4.0f, vp.y + 4.0f);
    cone.lineTo (vp.x + 4.0f, vp.y + 4.0f);
    cone.lineTo (f.X (0.65f), f.Y (1.0f));
    cone.closeSubPath();
    juce::ColourGradient hl (k.light.withAlpha (0.35f), 0.0f, f.Y (1.0f), k.light.withAlpha (0.02f), 0.0f, vp.y, false);
    g.setGradientFill (hl);
    g.fillPath (cone);

    for (int i = 0; i < 14; ++i)
    {
        const float d = approach (p.beat * 0.5f, (float) i / 14.0f);
        for (int side = -1; side <= 1; side += 2)
        {
            const float x = vp.x + (float) side * f.W() * (0.08f + 0.9f * d);
            const float h = f.H() * (0.1f + 1.2f * d);
            pine (g, x, vp.y + (f.Y (1.0f) - vp.y) * d, h, k.sil.withAlpha (0.5f + 0.5f * d), 0.0f);
        }
    }
}

void tunnel (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const juce::Point<float> c (f.X (0.5f), f.Y (0.5f));

    if (p.variant == 1)
    {
        glow (g, c, f.H() * 0.3f, k.light.withAlpha (0.9f));
        g.setColour (k.light.withAlpha (0.85f));
        g.fillEllipse (c.x - f.H() * 0.08f, c.y - f.H() * 0.06f, f.H() * 0.16f, f.H() * 0.12f);
    }

    for (int i = 0; i < 12; ++i)
    {
        const float d = approach (p.beat, (float) i / 12.0f);
        const float rx = f.W() * (0.03f + 0.7f * d), ry = f.H() * (0.025f + 0.75f * d);
        g.setColour (k.light.withAlpha (0.08f + 0.25f * d));
        g.drawEllipse (c.x - rx, c.y - ry, rx * 2.0f, ry * 2.0f, 1.0f + 3.0f * d);
        glow (g, { c.x, c.y - ry }, f.H() * 0.05f * d + 1.0f, k.warm.withAlpha (0.4f + 0.5f * d));
    }
}

void cityStreet (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const float horizon = 0.55f;
    nightSky (g, f, p, horizon);
    const juce::Point<float> vp (f.X (0.5f), f.Y (horizon));

    for (int side = -1; side <= 1; side += 2)
    {
        for (int b = 0; b < 6; ++b)
        {
            const float d0 = (float) b / 6.0f, d1 = (float) (b + 1) / 6.0f;
            const float x0 = vp.x + (float) side * f.W() * (0.06f + 0.5f * d0 * d0);
            const float x1 = vp.x + (float) side * f.W() * (0.06f + 0.5f * d1 * d1);
            const float top0 = vp.y - f.H() * (0.15f + 0.6f * d0) * (0.7f + 0.5f * hash (p.seed, b + side * 10));
            const float top1 = vp.y - f.H() * (0.15f + 0.6f * d1) * (0.7f + 0.5f * hash (p.seed, b + side * 10));
            juce::Path bld;
            bld.startNewSubPath (x0, vp.y + (f.Y (1.0f) - vp.y) * d0 * d0);
            bld.lineTo (x0, top0);
            bld.lineTo (x1, top1);
            bld.lineTo (x1, vp.y + (f.Y (1.0f) - vp.y) * d1 * d1);
            bld.closeSubPath();
            g.setColour (k.sil.withAlpha (0.92f));
            g.fillPath (bld);

            for (int w = 0; w < 6; ++w)
            {
                if (hash (p.seed, b * 31 + w + side * 7) < 0.8f)
                    continue;
                const float wx = x0 + (x1 - x0) * (0.2f + 0.6f * hash (p.seed, w + b));
                const float wy = juce::jmin (top0, top1) + (vp.y - juce::jmin (top0, top1)) * hash (p.seed, w * 3 + b);
                const auto col = p.variant == 1 && hash (p.seed, w + 99) > 0.5f ? k.accent : k.warm;
                g.setColour (col.withAlpha (0.8f));
                g.fillRect (wx, wy, 3.0f + 4.0f * d1, 4.0f + 4.0f * d1);
            }
        }
    }

    if (p.variant == 0)
    {
        // a traffic light blinking amber at 3 AM
        const float on = frac (p.beat * 0.5f) < 0.5f ? 1.0f : 0.15f;
        g.setColour (k.sil);
        g.drawLine (f.X (0.62f), f.Y (0.9f), f.X (0.62f), f.Y (0.55f), 2.0f);
        glow (g, { f.X (0.62f), f.Y (0.55f) }, f.H() * 0.05f, k.warm.withAlpha (0.8f * on));
    }
    else
    {
        // neon signs (pure shapes) and their reflections in the wet street
        for (int i = 0; i < 4; ++i)
        {
            const float x = f.X (i < 2 ? 0.2f + 0.08f * (float) i : 0.72f + 0.08f * (float) (i - 2));
            const float y = f.Y (0.3f + 0.1f * hash (p.seed, i + 70));
            const auto col = i % 2 == 0 ? k.accent : k.warm;
            const float buzz = hash (p.seed, (int) (p.t * 10.0f) + i) > 0.97f ? 0.2f : 1.0f;
            juce::Path sign;
            if (i % 2 == 0) sign.addRoundedRectangle (x - 12.0f, y - 6.0f, 24.0f, 12.0f, 4.0f);
            else            sign.addEllipse (x - 8.0f, y - 8.0f, 16.0f, 16.0f);
            glowStroke (g, sign, col, 1.5f, buzz);
            reflection (g, x, f.Y (0.82f), 14.0f, f.H() * 0.15f, col.withAlpha (0.35f * buzz), p.t, p.seed + (juce::uint32) i);
        }
        rain (g, f, p.seed, p.t, 120, k.light.withAlpha (0.25f), 0.1f);
    }
}

void aerialCity (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    g.setColour (k.deep.withAlpha (0.4f));
    g.fillRect (f.r);

    G::ScopedSaveState state (g);
    g.addTransform (juce::AffineTransform::rotation (0.35f, f.X (0.5f), f.Y (0.5f)));
    const float cell = f.W() * 0.06f;

    for (int i = -10; i <= 26; ++i)
    {
        const float x = f.X (0.0f) + (float) i * cell;
        const float y = f.Y (0.0f) + (float) i * cell * 0.7f;
        g.setColour (k.warm.withAlpha (0.18f));
        g.drawLine (x, f.Y (-0.6f), x, f.Y (1.6f), 1.0f);
        g.drawLine (f.X (-0.6f), y, f.X (1.6f), y, 1.0f);

        for (int j = 0; j < 4; ++j)
        {
            const float u = frac (hash (p.seed, i * 4 + j) + p.beat * 0.05f * (j % 2 == 0 ? 1.0f : -1.0f));
            g.setColour ((j % 2 == 0 ? k.light : k.warm).withAlpha (0.8f));
            g.fillEllipse (x - 1.5f, f.Y (-0.6f) + u * f.H() * 2.2f, 3.0f, 3.0f);
        }
    }

    if (p.variant == 1)
    {
        juce::Path river;
        river.startNewSubPath (f.X (-0.5f), f.Y (0.3f));
        river.cubicTo (f.X (0.2f), f.Y (0.8f), f.X (0.6f), f.Y (0.0f), f.X (1.5f), f.Y (0.6f));
        g.setColour (k.deep.withAlpha (0.95f));
        g.strokePath (river, juce::PathStrokeType (cell * 0.9f));
        g.setColour (k.light.withAlpha (0.12f));
        g.strokePath (river, juce::PathStrokeType (1.0f));
    }
    else if (p.variant == 2)
    {
        juce::Path loops;
        loops.addEllipse (f.X (0.3f), f.Y (0.3f), f.W() * 0.2f, f.H() * 0.25f);
        loops.addEllipse (f.X (0.5f), f.Y (0.45f), f.W() * 0.2f, f.H() * 0.25f);
        glowStroke (g, loops, k.warm, 1.5f, 0.6f);
    }
}

void telephonePoles (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.65f);
    g.setColour (k.mid.withAlpha (0.5f));
    g.fillPath (ridge (f, p.seed, 0.7f, 0.1f, 0.2f, 40));
    g.setColour (k.sil.withAlpha (0.9f));
    g.fillRect (f.X (0.0f), f.Y (0.85f), f.W(), f.H() * 0.15f);

    const float spacing = f.W() * 0.45f;
    const float offset = frac (p.beat * 0.5f) * spacing;
    for (int i = -1; i < 4; ++i)
    {
        const float x = f.X (1.0f) - (float) i * spacing - offset;
        g.setColour (k.sil);
        g.drawLine (x, f.Y (0.86f), x, f.Y (0.2f), 3.0f);
        g.drawLine (x - 20.0f, f.Y (0.25f), x + 20.0f, f.Y (0.25f), 2.0f);

        const float nx = x - spacing;
        for (int w = -1; w <= 1; w += 2)
        {
            juce::Path wire;
            wire.startNewSubPath (x + (float) w * 18.0f, f.Y (0.25f));
            wire.quadraticTo ((x + nx) * 0.5f, f.Y (0.33f), nx + (float) w * 18.0f, f.Y (0.25f));
            g.strokePath (wire, juce::PathStrokeType (0.8f));
        }
    }
}

void railway (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const float horizon = 0.45f;
    nightSky (g, f, p, horizon);
    const juce::Point<float> vp (f.X (0.5f), f.Y (horizon));

    for (int i = 0; i < 16; ++i)
    {
        const float d = approach (p.beat, (float) i / 16.0f);
        const float y = vp.y + (f.Y (1.0f) - vp.y) * d;
        const float w = f.W() * 0.5f * d;
        g.setColour (k.mid.withAlpha (0.3f + 0.5f * d));
        g.drawLine (vp.x - w, y, vp.x + w, y, 1.0f + 4.0f * d);
    }

    g.setColour (k.light.withAlpha (0.55f));
    g.drawLine (vp.x - 1.0f, vp.y, f.X (0.2f), f.Y (1.0f), 2.0f);
    g.drawLine (vp.x + 1.0f, vp.y, f.X (0.8f), f.Y (1.0f), 2.0f);

    const float on = frac (p.beat * 0.25f) < 0.5f ? 1.0f : 0.3f;
    g.setColour (k.sil);
    g.drawLine (f.X (0.62f), vp.y + f.H() * 0.05f, f.X (0.62f), vp.y - f.H() * 0.12f, 2.0f);
    glow (g, { f.X (0.62f), vp.y - f.H() * 0.12f }, f.H() * 0.03f, k.accent.withAlpha (on));
}

void endlessRoad (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const float horizon = 0.5f;
    nightSky (g, f, p, horizon);
    const juce::Point<float> vp (f.X (0.5f), f.Y (horizon));

    juce::Path road;
    road.startNewSubPath (vp.x - 1.0f, vp.y);
    road.lineTo (vp.x + 1.0f, vp.y);
    road.lineTo (f.X (0.8f), f.Y (1.0f));
    road.lineTo (f.X (0.2f), f.Y (1.0f));
    road.closeSubPath();
    g.setColour (k.mid.withAlpha (0.35f));
    g.fillPath (road);

    // a car's tail-lights receding into the dark
    const float d = 1.0f - std::fmod (p.t * 0.04f, 1.0f);
    const float dd = d * d;
    const float y = vp.y + (f.Y (1.0f) - vp.y) * dd;
    for (int s = -1; s <= 1; s += 2)
        glow (g, { vp.x + (float) s * f.W() * 0.06f * dd, y }, f.H() * 0.03f * dd + 1.5f, k.warm.interpolatedWith (k.accent, 0.1f));

    // the darkness closing in
    juce::ColourGradient dark (k.deep.withAlpha (0.0f), 0.0f, f.Y (0.7f), k.deep.withAlpha (0.8f), 0.0f, vp.y, false);
    g.setGradientFill (dark);
    g.fillRect (f.X (0.0f), vp.y - f.H() * 0.05f, f.W(), f.H() * 0.25f);
}

void bridgeLights (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const float water = 0.65f;
    nightSky (g, f, p, water);
    g.setColour (k.deep.withAlpha (0.55f));
    g.fillRect (f.X (0.0f), f.Y (water), f.W(), f.H() * (1.0f - water));

    const float deck = f.Y (0.55f);
    g.setColour (k.sil);
    g.fillRect (f.X (0.0f), deck, f.W(), 4.0f);
    for (int t = 0; t < 2; ++t)
    {
        const float tx = f.X (0.3f + 0.4f * (float) t);
        g.fillRect (tx - 3.0f, f.Y (0.2f), 6.0f, f.Y (water) - f.Y (0.2f));
    }

    juce::Path cable;
    cable.startNewSubPath (f.X (0.0f), f.Y (0.45f));
    cable.quadraticTo (f.X (0.15f), f.Y (0.5f), f.X (0.3f), f.Y (0.2f));
    cable.quadraticTo (f.X (0.5f), f.Y (0.6f), f.X (0.7f), f.Y (0.2f));
    cable.quadraticTo (f.X (0.85f), f.Y (0.5f), f.X (1.0f), f.Y (0.45f));
    g.strokePath (cable, juce::PathStrokeType (1.5f));

    for (int i = 0; i < 24; ++i)
    {
        const float x = f.X ((float) i / 23.0f);
        const float on = 0.6f + 0.4f * std::sin (p.beat * juce::MathConstants<float>::pi + (float) i * 0.5f);
        glow (g, { x, deck - 2.0f }, 4.0f, k.warm.withAlpha (on));
        reflection (g, x, f.Y (water) + 2.0f, 3.0f, f.H() * 0.15f, k.warm.withAlpha (0.25f * on), p.t, p.seed + (juce::uint32) i);
    }
}

void levelCrossing (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.7f);
    g.setColour (k.sil.withAlpha (0.9f));
    g.fillRect (f.X (0.0f), f.Y (0.75f), f.W(), f.H() * 0.25f);
    g.setColour (k.light.withAlpha (0.4f));
    g.drawLine (f.X (0.0f), f.Y (0.8f), f.X (1.0f), f.Y (0.8f), 2.0f);
    g.drawLine (f.X (0.0f), f.Y (0.84f), f.X (1.0f), f.Y (0.84f), 2.0f);

    const float x = f.X (0.45f);
    g.setColour (k.sil);
    g.drawLine (x, f.Y (0.76f), x, f.Y (0.4f), 3.0f);
    g.drawLine (x - 15.0f, f.Y (0.42f), x + 15.0f, f.Y (0.48f), 3.0f);
    g.drawLine (x - 15.0f, f.Y (0.48f), x + 15.0f, f.Y (0.42f), 3.0f);
    g.drawLine (x, f.Y (0.58f), x + f.W() * 0.3f, f.Y (0.58f), 3.0f);

    const bool left = frac (p.beat * 0.5f) < 0.5f;
    glow (g, { x - 12.0f, f.Y (0.53f) }, f.H() * 0.05f, k.warm.interpolatedWith (k.accent, 0.1f).withAlpha (left ? 0.95f : 0.1f));
    glow (g, { x + 12.0f, f.Y (0.53f) }, f.H() * 0.05f, k.warm.interpolatedWith (k.accent, 0.1f).withAlpha (left ? 0.1f : 0.95f));
}

void petrolStation (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.7f);
    stars (g, f, p.seed, 50, 0.5f, k.light.withAlpha (0.4f), p.t);

    const auto canopy = R (f.X (0.3f), f.Y (0.4f), f.W() * 0.4f, f.H() * 0.05f);
    juce::Path pool;
    pool.startNewSubPath (canopy.getX(), canopy.getBottom());
    pool.lineTo (canopy.getRight(), canopy.getBottom());
    pool.lineTo (canopy.getRight() + f.W() * 0.08f, f.Y (0.85f));
    pool.lineTo (canopy.getX() - f.W() * 0.08f, f.Y (0.85f));
    pool.closeSubPath();
    juce::ColourGradient lg (k.light.withAlpha (0.35f), 0.0f, canopy.getBottom(), k.light.withAlpha (0.05f), 0.0f, f.Y (0.85f), false);
    g.setGradientFill (lg);
    g.fillPath (pool);

    g.setColour (k.light.withAlpha (0.8f));
    g.fillRect (canopy.getX(), canopy.getBottom() - 2.0f, canopy.getWidth(), 2.0f);
    g.setColour (k.sil);
    g.fillRect (canopy);
    g.fillRect (canopy.getX() + canopy.getWidth() * 0.15f, canopy.getBottom(), 4.0f, f.Y (0.85f) - canopy.getBottom());
    g.fillRect (canopy.getRight() - canopy.getWidth() * 0.15f, canopy.getBottom(), 4.0f, f.Y (0.85f) - canopy.getBottom());
    for (int i = 0; i < 2; ++i)
        g.fillRect (canopy.getX() + canopy.getWidth() * (0.35f + 0.25f * (float) i), f.Y (0.72f), canopy.getWidth() * 0.06f, f.Y (0.85f) - f.Y (0.72f));

    g.setColour (k.sil.withAlpha (0.9f));
    g.fillRect (f.X (0.0f), f.Y (0.85f), f.W(), f.H() * 0.15f);
}
} // namespace

void addRoadScenes (std::vector<SceneSpec>& v)
{
    //            name                               category          fn              var  ambient rhythm sparse dark intense  rare
    v.push_back ({ "Night motorway",                 Category::Roads,  motorway,       0, { 0.2f, 1.0f, 0.2f, 0.3f, 0.7f }, false });
    v.push_back ({ "Wet motorway",                   Category::Roads,  motorway,       1, { 0.3f, 1.0f, 0.3f, 0.3f, 0.6f }, false });
    v.push_back ({ "Following the convoy",           Category::Roads,  motorway,       2, { 0.2f, 1.0f, 0.2f, 0.4f, 0.8f }, false });
    v.push_back ({ "Lights passing overhead",        Category::Roads,  overheadLights, 0, { 0.2f, 1.0f, 0.2f, 0.3f, 0.8f }, false });
    v.push_back ({ "Dark forest road",               Category::Roads,  forestRoad,     0, { 0.3f, 0.9f, 0.2f, 0.8f, 0.6f }, false });
    v.push_back ({ "Tunnel",                         Category::Roads,  tunnel,         0, { 0.2f, 1.0f, 0.1f, 0.5f, 0.9f }, false });
    v.push_back ({ "Light at the end of the tunnel", Category::Roads,  tunnel,         1, { 0.3f, 0.9f, 0.2f, 0.4f, 0.7f }, false });
    v.push_back ({ "Empty street, 3 AM",             Category::Roads,  cityStreet,     0, { 0.4f, 0.5f, 0.7f, 0.5f, 0.2f }, false });
    v.push_back ({ "Neon in the rain",               Category::Roads,  cityStreet,     1, { 0.3f, 0.8f, 0.4f, 0.4f, 0.6f }, false });
    v.push_back ({ "City from above",                Category::Roads,  aerialCity,     0, { 0.4f, 0.8f, 0.3f, 0.3f, 0.5f }, false });
    v.push_back ({ "River through the city",         Category::Roads,  aerialCity,     1, { 0.5f, 0.7f, 0.3f, 0.3f, 0.4f }, false });
    v.push_back ({ "Interchange",                    Category::Roads,  aerialCity,     2, { 0.3f, 0.9f, 0.2f, 0.3f, 0.6f }, false });
    v.push_back ({ "Telephone poles",                Category::Roads,  telephonePoles, 0, { 0.4f, 0.9f, 0.4f, 0.3f, 0.3f }, false });
    v.push_back ({ "Railway at night",               Category::Roads,  railway,        0, { 0.3f, 1.0f, 0.3f, 0.5f, 0.6f }, false });
    v.push_back ({ "Road into darkness",             Category::Roads,  endlessRoad,    0, { 0.5f, 0.4f, 0.7f, 0.8f, 0.2f }, false });
    v.push_back ({ "Bridge lights",                  Category::Roads,  bridgeLights,   0, { 0.6f, 0.6f, 0.5f, 0.3f, 0.3f }, false });
    v.push_back ({ "Level crossing",                 Category::Roads,  levelCrossing,  0, { 0.2f, 0.9f, 0.4f, 0.6f, 0.5f }, false });
    v.push_back ({ "Petrol station, nowhere",        Category::Roads,  petrolStation,  0, { 0.5f, 0.3f, 0.8f, 0.5f, 0.1f }, true  });
}
}

// =================================================================================================
//  DREAMLIKE HUMAN MOMENTS - small figures, big spaces, quiet light
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

void nightSky (G& g, const Frame& f, const P& p, float horizon)
{
    const auto& k = p.ink;
    sky (g, f, k.deep.withAlpha (0.0f), k.haze.withAlpha (0.5f), k.deep.withAlpha (0.55f), horizon);
}

void forestLane (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.5f);
    const juce::Point<float> vp (f.X (0.5f), f.Y (0.5f));

    glow (g, vp, f.H() * 0.3f, k.haze.withAlpha (0.6f));

    juce::Path lane;
    lane.startNewSubPath (vp.x - f.W() * 0.01f, vp.y);
    lane.lineTo (vp.x + f.W() * 0.01f, vp.y);
    lane.lineTo (f.X (0.75f), f.Y (1.0f));
    lane.lineTo (f.X (0.25f), f.Y (1.0f));
    lane.closeSubPath();
    g.setColour (k.mid.withAlpha (0.35f));
    g.fillPath (lane);

    for (int i = 0; i < 9; ++i)
    {
        const float d = (float) i / 9.0f;
        const float s = 1.0f - d * 0.9f;
        for (int side = -1; side <= 1; side += 2)
        {
            const float x = vp.x + (float) side * f.W() * (0.08f + 0.5f * s);
            const float sway = std::sin (p.t * 0.5f + (float) i) * 0.3f;
            const float trunkW = f.W() * 0.02f * s;
            g.setColour (k.sil.interpolatedWith (k.mid, d * 0.7f).withAlpha (0.95f));
            g.fillRect (x - trunkW * 0.5f, f.Y (0.0f), trunkW, vp.y + (f.Y (1.0f) - vp.y) * s - f.Y (0.0f));
            softEllipse (g, { x + sway * 10.0f * s, f.Y (0.15f) }, f.W() * 0.12f * s, f.H() * 0.18f, k.sil.withAlpha (0.6f * s));
        }
    }

    fogBand (g, f, 0.55f, 0.15f, k.haze.withAlpha (0.35f), p.t, p.seed);

    const float walk = p.t * 0.012f;
    const float depth = 0.35f + std::fmod (walk, 0.4f);
    const float h = f.H() * 0.28f * (1.0f - depth);
    const float gy = vp.y + (f.Y (1.0f) - vp.y) * (1.0f - depth);
    if (p.variant == 0)
    {
        figure (g, vp.x - h * 0.13f, gy, h, k.sil, p.t * 3.0f, true);
        figure (g, vp.x + h * 0.13f, gy, h * 0.94f, k.sil, p.t * 3.0f + 0.4f, true);
    }
    else
        figure (g, vp.x, gy, h, k.sil, p.t * 3.0f, true);
}

void benchDistant (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.72f);
    stars (g, f, p.seed, 60, 0.6f, k.light.withAlpha (0.5f), p.t);
    g.setColour (k.sil.withAlpha (0.85f));
    g.fillPath (ridge (f, p.seed, 0.8f, 0.04f, 0.0f, 30));

    const float bx = f.X (0.58f), gy = f.Y (0.78f), u = f.H() * 0.02f;
    streetlight (g, bx - u * 6.0f, gy, u * 10.0f, k.sil, k.warm, 0.7f + 0.3f * p.mid, true);
    g.setColour (k.sil);
    g.fillRect (bx - u * 2.5f, gy - u * 1.6f, u * 5.0f, u * 0.4f);
    g.fillRect (bx - u * 2.5f, gy - u * 2.8f, u * 5.0f, u * 0.3f);
    g.fillRect (bx - u * 2.3f, gy - u * 1.6f, u * 0.3f, u * 1.6f);
    g.fillRect (bx + u * 2.0f, gy - u * 1.6f, u * 0.3f, u * 1.6f);

    if (p.variant == 0)
    {
        figure (g, bx - u * 1.0f, gy - u * 0.2f, u * 3.6f, k.sil, 0.0f, false);
        figure (g, bx + u * 0.9f + (std::sin (p.t * 0.3f) > 0.6f ? -u * 0.2f : 0.0f), gy - u * 0.2f, u * 3.4f, k.sil, 0.0f, false);
    }
    else
        figure (g, bx, gy - u * 0.2f, u * 3.6f, k.sil, 0.0f, false);

    // a large tree
    softEllipse (g, { f.X (0.25f), f.Y (0.55f) }, f.W() * 0.12f, f.H() * 0.18f, k.sil.withAlpha (0.9f));
    g.setColour (k.sil);
    g.fillRect (f.X (0.245f), f.Y (0.6f), f.W() * 0.012f, f.H() * 0.2f);
}

void streetlightFigure (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.85f);
    const float gy = f.Y (0.86f);
    const float flicker = std::fmod (p.t, 7.0f) < 0.12f ? 0.4f : 1.0f;
    streetlight (g, f.X (0.45f), gy, f.H() * 0.6f, k.sil, k.warm, (0.75f + 0.25f * p.mid) * flicker, true);
    figure (g, f.X (0.55f), gy, f.H() * 0.22f, k.sil, 0.0f, false);
    g.setColour (k.deep.withAlpha (0.5f));
    g.fillEllipse (f.X (0.5f), gy - 2.0f, f.W() * 0.12f, 6.0f);

    if (p.variant == 1)
    {
        rain (g, f, p.seed, p.t, 120, k.light.withAlpha (0.35f), 0.3f);
        reflection (g, f.X (0.52f), gy + 4.0f, f.W() * 0.06f, f.H() * 0.12f, k.warm.withAlpha (0.35f), p.t, p.seed);
    }
}

void rainWindow (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const auto win = R (f.X (0.3f), f.Y (0.12f), f.W() * 0.5f, f.H() * 0.72f);

    // blurred city beyond the glass
    g.setColour (k.haze.withAlpha (0.35f));
    g.fillRect (win);
    for (int i = 0; i < 18; ++i)
    {
        const juce::Point<float> c (win.getX() + win.getWidth() * hash (p.seed, i), win.getY() + win.getHeight() * (0.4f + 0.6f * hash (p.seed, i + 40)));
        glow (g, c, win.getWidth() * (0.03f + 0.05f * hash (p.seed, i + 80)), (hash (p.seed, i + 9) > 0.5f ? k.warm : k.accent).withAlpha (0.5f));
    }

    // drops on the glass
    for (int i = 0; i < 60; ++i)
    {
        const float speed = 0.03f + 0.08f * hash (p.seed, i + 200);
        const float y = std::fmod (hash (p.seed, i + 300) + p.t * speed, 1.0f);
        const float x = win.getX() + win.getWidth() * hash (p.seed, i + 400);
        const float yy = win.getY() + win.getHeight() * y;
        g.setColour (k.light.withAlpha (0.35f));
        g.drawLine (x, yy, x, yy - win.getHeight() * 0.05f * hash (p.seed, i), 0.8f);
        g.fillEllipse (x - 1.2f, yy - 1.2f, 2.4f, 2.4f);
    }

    // window frame and the room
    g.setColour (k.sil.withAlpha (0.95f));
    g.fillRect (f.X (0.0f), f.Y (0.0f), win.getX() - f.X (0.0f), f.H());
    g.fillRect (win.getRight(), f.Y (0.0f), f.X (1.0f) - win.getRight(), f.H());
    g.fillRect (win.getX(), f.Y (0.0f), win.getWidth(), win.getY() - f.Y (0.0f));
    g.fillRect (win.getX(), win.getBottom(), win.getWidth(), f.Y (1.0f) - win.getBottom());
    g.fillRect (win.getCentreX() - 2.0f, win.getY(), 4.0f, win.getHeight());
    g.fillRect (win.getX(), win.getCentreY() - 2.0f, win.getWidth(), 4.0f);

    figure (g, f.X (0.2f), f.Y (1.05f), f.H() * 0.8f, k.sil.interpolatedWith (k.mid, 0.15f), 0.0f, false);
}

void tinyWalker (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.62f);

    if (p.variant == 0)
    {
        stars (g, f, p.seed, 80, 0.55f, k.light.withAlpha (0.5f), p.t);
        g.setColour (k.mid.withAlpha (0.5f));
        g.fillPath (ridge (f, p.seed, 0.7f, 0.12f, 0.0f, 40));
        g.setColour (k.sil.withAlpha (0.85f));
        g.fillPath (ridge (f, p.seed + 5u, 0.82f, 0.08f, 0.0f, 40));
    }
    else if (p.variant == 1)
    {
        g.setColour (k.mid.withAlpha (0.6f));
        g.fillPath (ridge (f, p.seed, 0.6f, 0.35f, 0.8f, 60));
        g.setColour (k.light.withAlpha (0.3f));
        g.fillRect (f.X (0.0f), f.Y (0.72f), f.W(), f.H() * 0.28f);
        snow (g, f, p.seed, p.t, 70, k.light.withAlpha (0.5f));
    }
    else
    {
        moon (g, { f.X (0.5f), f.Y (0.4f) }, f.H() * 0.22f, k.light.withAlpha (0.55f));
        g.setColour (k.haze.withAlpha (0.3f));
        g.fillRect (f.X (0.0f), f.Y (0.68f), f.W(), f.H() * 0.32f);
    }

    const float x = f.X (0.2f + std::fmod (p.t * 0.006f, 0.6f));
    const float gy = f.Y (p.variant == 1 ? 0.76f : 0.8f);
    figure (g, x, gy, f.H() * 0.05f, k.sil, p.t * 3.5f, true);
}

void trainFog (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.6f);
    g.setColour (k.mid.withAlpha (0.5f));
    g.fillPath (ridge (f, p.seed, 0.6f, 0.12f, 0.2f, 40));

    const float trackY = f.Y (p.variant == 1 ? 0.55f : 0.68f);
    if (p.variant == 1)
    {
        g.setColour (k.sil.withAlpha (0.9f));
        g.fillRect (f.X (0.0f), trackY + 4.0f, f.W(), f.H() * 0.02f);
        for (int i = 0; i < 7; ++i)
        {
            const float x = f.X ((float) i / 6.0f);
            juce::Path arch;
            arch.addRectangle (x - f.W() * 0.012f, trackY + 4.0f, f.W() * 0.024f, f.Y (1.0f) - trackY);
            g.fillPath (arch);
        }
    }

    const float x0 = f.X (1.1f) - std::fmod (p.t * 0.05f, 1.6f) * f.W();
    const float carW = f.W() * 0.09f, carH = f.H() * 0.04f;
    for (int c = 0; c < 6; ++c)
    {
        const float x = x0 + (float) c * carW * 1.05f;
        g.setColour (k.sil.withAlpha (0.95f));
        g.fillRect (x, trackY - carH, carW, carH);
        for (int w = 0; w < 5; ++w)
        {
            g.setColour (k.warm.withAlpha (0.85f));
            g.fillRect (x + carW * (0.1f + 0.18f * (float) w), trackY - carH * 0.75f, carW * 0.1f, carH * 0.35f);
        }
    }
    glow (g, { x0 - carW * 0.1f, trackY - carH * 0.5f }, carW * 0.8f, k.light.withAlpha (0.4f));

    fogBand (g, f, 0.62f, 0.2f, k.haze.withAlpha (0.55f), p.t, p.seed);
    // fog swallowing the train on the left
    juce::ColourGradient eat (k.haze.withAlpha (0.8f), f.X (0.0f), 0.0f, k.haze.withAlpha (0.0f), f.X (0.35f), 0.0f, false);
    g.setGradientFill (eat);
    g.fillRect (f.X (0.0f), trackY - f.H() * 0.15f, f.W() * 0.35f, f.H() * 0.25f);
}

void station (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.75f);
    const float gy = f.Y (0.78f);

    g.setColour (k.mid.withAlpha (0.45f));
    g.fillRect (f.X (0.0f), gy, f.W(), f.H() * 0.04f);
    g.setColour (k.sil.withAlpha (0.9f));
    g.fillRect (f.X (0.0f), gy + f.H() * 0.04f, f.W(), f.H() * 0.18f);

    if (p.variant == 0)
    {
        // canopy and lamps
        g.fillRect (f.X (0.1f), f.Y (0.28f), f.W() * 0.8f, f.H() * 0.03f);
        for (int i = 0; i < 4; ++i)
        {
            const float x = f.X (0.18f + 0.22f * (float) i);
            g.setColour (k.sil);
            g.fillRect (x, f.Y (0.31f), 3.0f, gy - f.Y (0.31f));
            const float on = (i == 2 && std::fmod (p.t, 5.0f) < 0.2f) ? 0.2f : 1.0f;
            glow (g, { x + 1.5f, f.Y (0.33f) }, f.H() * 0.12f, k.warm.withAlpha (0.45f * on));
        }
        // clock
        const juce::Point<float> c (f.X (0.62f), f.Y (0.4f));
        g.setColour (k.light.withAlpha (0.7f));
        g.drawEllipse (c.x - 9.0f, c.y - 9.0f, 18.0f, 18.0f, 1.2f);
        g.drawLine (c.x, c.y, c.x + std::sin (p.t * 0.05f) * 7.0f, c.y - std::cos (p.t * 0.05f) * 7.0f, 1.0f);
        g.drawLine (c.x, c.y, c.x + 4.0f, c.y + 2.0f, 1.0f);
        figure (g, f.X (0.4f), gy, f.H() * 0.17f, k.sil, 0.0f, false);
    }
    else
    {
        // a lone bus shelter
        const auto shelter = R (f.X (0.4f), f.Y (0.48f), f.W() * 0.2f, gy - f.Y (0.48f));
        glow (g, shelter.getCentre(), shelter.getWidth() * 0.7f, k.accent.withAlpha (0.3f));
        g.setColour (k.light.withAlpha (0.18f));
        g.fillRect (shelter);
        g.setColour (k.sil);
        g.fillRect (shelter.getX(), shelter.getY(), shelter.getWidth(), 4.0f);
        g.fillRect (shelter.getX(), shelter.getY(), 3.0f, shelter.getHeight());
        g.fillRect (shelter.getRight() - 3.0f, shelter.getY(), 3.0f, shelter.getHeight());
        figure (g, shelter.getCentreX(), gy, f.H() * 0.17f, k.sil, 0.0f, false);
        rain (g, f, p.seed, p.t, 90, k.light.withAlpha (0.3f), 0.2f);
    }
}

void beach (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const float horizon = 0.55f;
    nightSky (g, f, p, horizon);
    moon (g, { f.X (0.7f), f.Y (0.25f) }, f.H() * 0.045f, k.light.withAlpha (0.85f));
    g.setColour (k.deep.withAlpha (0.55f));
    g.fillRect (f.X (0.0f), f.Y (horizon), f.W(), f.H() * (1.0f - horizon));
    reflection (g, f.X (0.7f), f.Y (horizon) + 2.0f, f.W() * 0.05f, f.H() * 0.25f, k.light.withAlpha (0.45f), p.t, p.seed);

    for (int i = 0; i < 3; ++i)
    {
        const float y = f.Y (0.78f + 0.04f * (float) i + 0.01f * std::sin (p.t * 0.6f + (float) i));
        g.setColour (k.light.withAlpha (0.25f - 0.06f * (float) i));
        g.drawLine (f.X (0.0f), y, f.X (1.0f), y + 4.0f, 1.0f);
    }
    g.setColour (k.mid.withAlpha (0.4f));
    g.fillRect (f.X (0.0f), f.Y (0.86f), f.W(), f.H() * 0.14f);

    const float x = f.X (0.15f + std::fmod (p.t * 0.008f, 0.5f));
    for (int i = 1; i < 12; ++i)
    {
        g.setColour (k.sil.withAlpha (0.35f * (1.0f - (float) i / 12.0f)));
        g.fillEllipse (x - (float) i * f.W() * 0.012f, f.Y (0.9f) + (float) (i % 2) * 3.0f, 3.0f, 2.0f);
    }
    if (p.variant == 0)
    {
        figure (g, x, f.Y (0.9f), f.H() * 0.12f, k.sil, p.t * 3.0f, true);
        figure (g, x + f.H() * 0.035f, f.Y (0.9f), f.H() * 0.115f, k.sil, p.t * 3.0f + 0.5f, true);
    }
    else
        figure (g, x, f.Y (0.9f), f.H() * 0.12f, k.sil, p.t * 3.0f, true);
}

void litHouse (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.65f);
    stars (g, f, p.seed, 70, 0.6f, k.light.withAlpha (0.5f), p.t);

    const float gy = f.Y (p.variant == 1 ? 0.62f : 0.72f);
    juce::Path hill;
    hill.startNewSubPath (f.X (0.0f), f.Y (1.0f));
    hill.lineTo (f.X (0.0f), gy + f.H() * 0.08f);
    hill.quadraticTo (f.X (0.5f), gy - f.H() * 0.05f, f.X (1.0f), gy + f.H() * 0.1f);
    hill.lineTo (f.X (1.0f), f.Y (1.0f));
    hill.closeSubPath();
    g.setColour (k.sil.withAlpha (0.92f));
    g.fillPath (hill);

    const float hx = f.X (0.55f), hy = gy - f.H() * 0.012f, w = f.W() * 0.06f;
    g.fillRect (hx - w * 0.5f, hy - w * 0.7f, w, w * 0.7f);
    juce::Path roof;
    roof.addTriangle (hx - w * 0.62f, hy - w * 0.7f, hx + w * 0.62f, hy - w * 0.7f, hx, hy - w * 1.15f);
    g.fillPath (roof);
    const float lamp = 0.85f + 0.15f * std::sin (p.t * 0.5f);
    glow (g, { hx + w * 0.15f, hy - w * 0.4f }, w * 0.7f, k.warm.withAlpha (0.45f * lamp));
    g.setColour (k.warm.withAlpha (lamp));
    g.fillRect (hx + w * 0.08f, hy - w * 0.48f, w * 0.16f, w * 0.16f);

    softEllipse (g, { hx - w * 1.8f, hy - w * 1.1f }, w * 0.7f, w * 1.0f, k.sil.withAlpha (0.95f));

    if (p.variant == 1)
    {
        g.setColour (k.deep.withAlpha (0.55f));
        g.fillRect (f.X (0.0f), f.Y (0.75f), f.W(), f.H() * 0.25f);
        reflection (g, hx + w * 0.16f, f.Y (0.76f), w * 0.3f, f.H() * 0.18f, k.warm.withAlpha (0.5f), p.t, p.seed);
    }
}

void stargazer (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.8f);

    // milky way band
    {
        G::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::rotation (-0.5f, f.X (0.5f), f.Y (0.4f)));
        softEllipse (g, { f.X (0.5f), f.Y (0.4f) }, f.W() * 0.7f, f.H() * 0.1f, k.haze.withAlpha (0.45f));
    }
    stars (g, f, p.seed, 220, 0.75f, k.light.withAlpha (0.7f), p.t);

    juce::Path hill;
    hill.startNewSubPath (f.X (0.0f), f.Y (1.0f));
    hill.lineTo (f.X (0.0f), f.Y (0.86f));
    hill.quadraticTo (f.X (0.35f), f.Y (0.74f), f.X (0.7f), f.Y (0.86f));
    hill.lineTo (f.X (1.0f), f.Y (0.9f));
    hill.lineTo (f.X (1.0f), f.Y (1.0f));
    hill.closeSubPath();
    g.setColour (k.sil);
    g.fillPath (hill);
    figure (g, f.X (0.35f), f.Y (0.785f), f.H() * 0.12f, k.sil, 0.0f, false);
}

void umbrella (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.75f);
    rain (g, f, p.seed, p.t, 160, k.light.withAlpha (0.3f), 0.15f);

    const float gy = f.Y (0.82f);
    streetlight (g, f.X (0.75f), gy, f.H() * 0.55f, k.sil, k.warm, 0.8f, true);
    g.setColour (k.deep.withAlpha (0.4f));
    g.fillRect (f.X (0.0f), gy, f.W(), f.H() * 0.18f);
    reflection (g, f.X (0.82f), gy + 3.0f, f.W() * 0.05f, f.H() * 0.15f, k.warm.withAlpha (0.4f), p.t, p.seed);

    const float x = f.X (0.42f), h = f.H() * 0.24f;
    figure (g, x - h * 0.1f, gy, h, k.sil, p.t * 2.5f, true);
    figure (g, x + h * 0.12f, gy, h * 0.94f, k.sil, p.t * 2.5f + 0.6f, true);

    juce::Path dome;
    dome.startNewSubPath (x - h * 0.45f, gy - h * 1.02f);
    dome.quadraticTo (x, gy - h * 1.5f, x + h * 0.45f, gy - h * 1.02f);
    dome.closeSubPath();
    g.setColour (k.sil);
    g.fillPath (dome);
    g.drawLine (x, gy - h * 1.3f, x, gy - h * 0.7f, 1.2f);
}

void rooftopCat (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.7f);
    moon (g, { f.X (0.65f), f.Y (0.3f) }, f.H() * 0.12f, k.light.withAlpha (0.75f));

    g.setColour (k.sil.withAlpha (0.95f));
    for (int i = 0; i < 6; ++i)
    {
        const float x = f.X ((float) i / 6.0f), w = f.W() / 6.0f;
        const float top = f.Y (0.62f + 0.12f * hash (p.seed, i));
        juce::Path roof;
        roof.startNewSubPath (x, f.Y (1.0f));
        roof.lineTo (x, top + f.H() * 0.06f);
        roof.lineTo (x + w * 0.5f, top);
        roof.lineTo (x + w, top + f.H() * 0.06f);
        roof.lineTo (x + w, f.Y (1.0f));
        roof.closeSubPath();
        g.fillPath (roof);
        if (hash (p.seed, i + 20) > 0.5f)
            g.fillRect (x + w * 0.65f, top - f.H() * 0.04f, w * 0.08f, f.H() * 0.06f);
    }

    // the cat, sitting on a ridge, tail swaying
    const float cx = f.X (0.5f + 1.0f / 12.0f), cy = f.Y (0.62f + 0.12f * hash (p.seed, 3)), s = f.H() * 0.05f;
    g.fillEllipse (cx - s * 0.5f, cy - s * 1.1f, s, s * 1.1f);
    g.fillEllipse (cx - s * 0.35f, cy - s * 1.65f, s * 0.7f, s * 0.6f);
    juce::Path ears;
    ears.addTriangle (cx - s * 0.33f, cy - s * 1.45f, cx - s * 0.12f, cy - s * 1.5f, cx - s * 0.28f, cy - s * 1.85f);
    ears.addTriangle (cx + s * 0.33f, cy - s * 1.45f, cx + s * 0.12f, cy - s * 1.5f, cx + s * 0.28f, cy - s * 1.85f);
    g.fillPath (ears);
    juce::Path tail;
    tail.startNewSubPath (cx + s * 0.4f, cy - s * 0.1f);
    tail.quadraticTo (cx + s * 1.2f, cy, cx + s * (1.0f + 0.3f * std::sin (p.t * 1.5f)), cy - s * 0.9f);
    g.strokePath (tail, juce::PathStrokeType (s * 0.15f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void lakeFigure (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const float shore = 0.6f;
    nightSky (g, f, p, shore);
    g.setColour (k.mid.withAlpha (0.55f));
    g.fillPath (ridge (f, p.seed, shore, 0.18f, 0.4f, 50));
    g.setColour (k.deep.withAlpha (0.55f));
    g.fillRect (f.X (0.0f), f.Y (shore), f.W(), f.H() * (1.0f - shore));

    {
        G::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::scale (1.0f, -0.6f, 0.0f, f.Y (shore)));
        g.setColour (k.mid.withAlpha (0.2f));
        g.fillPath (ridge (f, p.seed, shore, 0.18f, 0.4f, 50));
    }

    g.setColour (k.sil);
    g.fillRect (f.X (0.0f), f.Y (0.88f), f.W(), f.H() * 0.12f);
    figure (g, f.X (0.4f), f.Y (0.89f), f.H() * 0.2f, k.sil, 0.0f, false);
    fogBand (g, f, shore, 0.06f, k.haze.withAlpha (0.35f), p.t, p.seed);
}
} // namespace

void addHumanScenes (std::vector<SceneSpec>& v)
{
    //            name                              category          fn                 var  ambient rhythm sparse dark intense  rare
    v.push_back ({ "Couple on a forest lane",       Category::Human,  forestLane,        0, { 0.6f, 0.3f, 1.0f, 0.4f, 0.0f }, false });
    v.push_back ({ "Alone on the forest lane",      Category::Human,  forestLane,        1, { 0.6f, 0.3f, 1.0f, 0.5f, 0.0f }, false });
    v.push_back ({ "Two on a distant bench",        Category::Human,  benchDistant,      0, { 0.6f, 0.1f, 1.0f, 0.3f, 0.0f }, false });
    v.push_back ({ "One on a distant bench",        Category::Human,  benchDistant,      1, { 0.6f, 0.1f, 1.0f, 0.4f, 0.0f }, false });
    v.push_back ({ "Beneath the streetlight",       Category::Human,  streetlightFigure, 0, { 0.5f, 0.3f, 1.0f, 0.5f, 0.1f }, false });
    v.push_back ({ "Streetlight in the rain",       Category::Human,  streetlightFigure, 1, { 0.6f, 0.4f, 0.9f, 0.5f, 0.1f }, false });
    v.push_back ({ "Window in the rain",            Category::Human,  rainWindow,        0, { 0.7f, 0.3f, 1.0f, 0.4f, 0.0f }, false });
    v.push_back ({ "Walker in the desert",          Category::Human,  tinyWalker,        0, { 0.8f, 0.2f, 0.8f, 0.3f, 0.0f }, false });
    v.push_back ({ "Walker in the snowfield",       Category::Human,  tinyWalker,        1, { 0.8f, 0.2f, 0.8f, 0.3f, 0.0f }, false });
    v.push_back ({ "Walker under the great moon",   Category::Human,  tinyWalker,        2, { 0.8f, 0.2f, 0.7f, 0.4f, 0.1f }, true  });
    v.push_back ({ "Train into the fog",            Category::Human,  trainFog,          0, { 0.7f, 0.5f, 0.7f, 0.4f, 0.1f }, false });
    v.push_back ({ "Train across the viaduct",      Category::Human,  trainFog,          1, { 0.6f, 0.6f, 0.6f, 0.4f, 0.2f }, false });
    v.push_back ({ "Waiting at the empty station",  Category::Human,  station,           0, { 0.5f, 0.3f, 1.0f, 0.5f, 0.0f }, false });
    v.push_back ({ "The last bus stop",             Category::Human,  station,           1, { 0.5f, 0.3f, 1.0f, 0.5f, 0.0f }, false });
    v.push_back ({ "Two on the night beach",        Category::Human,  beach,             0, { 0.7f, 0.2f, 1.0f, 0.3f, 0.0f }, false });
    v.push_back ({ "Alone on the night beach",      Category::Human,  beach,             1, { 0.7f, 0.2f, 1.0f, 0.4f, 0.0f }, false });
    v.push_back ({ "One lit window",                Category::Human,  litHouse,          0, { 0.7f, 0.1f, 1.0f, 0.4f, 0.0f }, false });
    v.push_back ({ "House by the lake",             Category::Human,  litHouse,          1, { 0.8f, 0.1f, 0.9f, 0.4f, 0.0f }, false });
    v.push_back ({ "Watching the stars",            Category::Human,  stargazer,         0, { 0.9f, 0.1f, 0.9f, 0.3f, 0.0f }, false });
    v.push_back ({ "Under one umbrella",            Category::Human,  umbrella,          0, { 0.5f, 0.4f, 0.9f, 0.3f, 0.1f }, false });
    v.push_back ({ "Cat on the rooftops",           Category::Human,  rooftopCat,        0, { 0.6f, 0.2f, 0.8f, 0.4f, 0.1f }, true  });
    v.push_back ({ "Figure by the lake",            Category::Human,  lakeFigure,        0, { 0.8f, 0.1f, 1.0f, 0.4f, 0.0f }, false });
}
}

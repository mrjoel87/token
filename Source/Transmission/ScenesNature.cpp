// =================================================================================================
//  NATURE & MOVEMENT
// =================================================================================================
#include "SceneKit.h"
#include <algorithm>
#include <cmath>

namespace token::tx
{
namespace
{
using namespace kit;
using G = juce::Graphics;
using R = juce::Rectangle<float>;
using P = SceneParams;
constexpr float pi = juce::MathConstants<float>::pi;

void nightSky (G& g, const Frame& f, const P& p, float horizon)
{
    const auto& k = p.ink;
    sky (g, f, k.deep.withAlpha (0.0f), k.haze.withAlpha (0.5f), k.deep.withAlpha (0.55f), horizon);
}

void flock (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.7f);

    if (p.variant == 1)
    {
        g.setColour (k.mid.withAlpha (0.6f));
        g.fillPath (ridge (f, p.seed, 0.7f, 0.3f, 0.8f, 60));
        g.setColour (k.sil.withAlpha (0.8f));
        g.fillPath (ridge (f, p.seed + 4u, 0.85f, 0.15f, 0.5f, 60));
    }

    const int n = p.variant == 2 ? 13 : 24;
    const float travel = std::fmod (p.t * 0.035f, 1.6f) - 0.3f;
    for (int i = 0; i < n; ++i)
    {
        float x, y;
        if (p.variant == 2)
        {
            // a V of geese
            const int side = (i % 2) * 2 - 1;
            const float rank = (float) ((i + 1) / 2);
            x = travel - rank * 0.025f;
            y = 0.35f + (float) side * rank * 0.025f;
        }
        else
        {
            x = travel - 0.2f * hash (p.seed, i) + 0.01f * std::sin (p.t * 0.7f + (float) i);
            y = 0.25f + 0.25f * hash (p.seed, i + 50) + 0.02f * std::sin (p.t * 0.5f + (float) i * 0.7f);
        }
        bird (g, f.X (x), f.Y (y), f.H() * (0.012f + 0.01f * hash (p.seed, i + 90)),
              p.t * (6.0f + 4.0f * p.high) + (float) i * 1.7f, k.sil.interpolatedWith (k.light, 0.15f));
    }
}

void storm (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.85f);

    const bool lightning = p.variant == 1 && (p.kick > 0.6f || std::fmod (p.t, 6.0f) < 0.08f);
    if (lightning)
    {
        g.setColour (k.light.withAlpha (0.25f));
        g.fillRect (f.r);
    }

    for (int i = 0; i < 5; ++i)
    {
        const float drift = std::fmod (hash (p.seed, i) + p.t * (0.006f + 0.006f * hash (p.seed, i + 5)), 1.4f) - 0.2f;
        cloud (g, f.X (drift), f.Y (0.2f + 0.12f * hash (p.seed, i + 10)), f.W() * 0.4f, f.H() * 0.15f,
               k.sil.interpolatedWith (k.mid, 0.2f).withAlpha (0.9f), k.light.withAlpha (lightning ? 0.5f : 0.08f), p.seed + (juce::uint32) i);
    }

    if (p.variant == 1)
    {
        g.setColour (k.sil);
        g.fillPath (ridge (f, p.seed, 0.9f, 0.08f, 0.3f, 40));
        if (lightning)
        {
            juce::Path bolt;
            float x = f.X (0.3f + 0.4f * hash (p.seed, (int) (p.t * 0.3f))), y = f.Y (0.3f);
            bolt.startNewSubPath (x, y);
            for (int i = 0; i < 8; ++i)
            {
                x += (hash (p.seed, i + (int) p.t) - 0.5f) * f.W() * 0.06f;
                y += f.H() * 0.07f;
                bolt.lineTo (x, y);
            }
            glowStroke (g, bolt, k.light, 1.5f, 1.0f);
        }
    }

    if (p.variant == 2)
    {
        rain (g, f, p.seed, p.t, 200, k.light.withAlpha (0.3f), 0.25f);
        fogBand (g, f, 0.8f, 0.2f, k.haze.withAlpha (0.4f), p.t, p.seed);
    }
}

void forest (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.6f);
    moon (g, { f.X (0.25f), f.Y (0.25f) }, f.H() * 0.05f, k.light.withAlpha (0.7f));

    if (p.variant == 1)
    {
        g.setColour (k.mid.withAlpha (0.5f));
        g.fillPath (ridge (f, p.seed, 0.6f, 0.18f, 0.3f, 50));
        trees (g, f, p.seed, 50, 0.62f, 0.08f, 0.16f, k.sil.withAlpha (0.85f), p.t, 0.2f + p.bass);
    }
    else
    {
        trees (g, f, p.seed, 24, 0.75f, 0.3f, 0.5f, k.mid.withAlpha (0.55f), p.t, 0.4f + p.bass);
        fogBand (g, f, 0.72f, 0.12f, k.haze.withAlpha (0.35f), p.t, p.seed);
        trees (g, f, p.seed + 7u, 12, 1.0f, 0.55f, 0.95f, k.sil.withAlpha (0.95f), p.t, 0.6f + 1.5f * p.bass);
    }
}

void moonWater (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const float horizon = 0.58f;
    nightSky (g, f, p, horizon);
    stars (g, f, p.seed, 60, 0.5f, k.light.withAlpha (0.5f), p.t);
    const juce::Point<float> m (f.X (0.5f), f.Y (0.3f));
    moon (g, m, f.H() * 0.07f, k.light.withAlpha (0.9f));

    g.setColour (k.deep.withAlpha (0.55f));
    g.fillRect (f.X (0.0f), f.Y (horizon), f.W(), f.H() * (1.0f - horizon));

    for (int i = 0; i < 40; ++i)
    {
        const float d = (float) i / 40.0f;
        const float y = f.Y (horizon) + f.H() * (1.0f - horizon) * d * d;
        const float w = f.W() * (0.02f + 0.12f * d) * (0.4f + 0.8f * noise1 (p.seed + (juce::uint32) i, p.t * 1.2f));
        const float x = m.x + std::sin (p.t * 1.1f + (float) i * 0.9f) * f.W() * 0.02f * d;
        g.setColour (k.light.withAlpha ((0.55f - 0.35f * d) * (0.6f + 0.4f * p.high)));
        g.fillRect (x - w * 0.5f, y, w, juce::jmax (1.0f, 2.0f * d + 0.5f));
    }

    if (p.variant == 1)
    {
        g.setColour (k.sil);
        for (int i = 0; i < 30; ++i)
        {
            const float x = f.X (hash (p.seed, i) * 0.3f);
            const float h = f.H() * (0.1f + 0.15f * hash (p.seed, i + 30));
            const float sway = std::sin (p.t * 0.9f + (float) i) * f.W() * 0.006f;
            g.drawLine (x, f.Y (1.0f), x + sway, f.Y (1.0f) - h, 1.2f);
        }
    }
}

void underwater (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    sky (g, f, k.haze.withAlpha (0.45f), k.deep.withAlpha (0.55f), k.deep.withAlpha (0.8f), 0.3f);

    if (p.variant == 0)
    {
        for (int l = 0; l < 7; ++l)
        {
            juce::Path current;
            for (int i = 0; i <= 50; ++i)
            {
                const float u = (float) i / 50.0f;
                const float y = f.Y (0.15f + 0.11f * (float) l) + std::sin (u * 7.0f + p.t * (0.4f + 0.1f * (float) l) + (float) l) * f.H() * 0.03f;
                if (i == 0) current.startNewSubPath (f.X (u), y); else current.lineTo (f.X (u), y);
            }
            g.setColour (k.light.withAlpha (0.12f + 0.1f * p.mid));
            g.strokePath (current, juce::PathStrokeType (1.0f + (float) (l % 3)));
        }
        for (int i = 0; i < 70; ++i)
        {
            const float x = std::fmod (hash (p.seed, i) + p.t * 0.02f * (0.5f + hash (p.seed, i + 70)), 1.0f);
            const float y = hash (p.seed, i + 140) + 0.02f * std::sin (p.t + (float) i);
            g.setColour (k.light.withAlpha (0.3f + 0.4f * hash (p.seed, i + 210)));
            g.fillEllipse (f.X (x), f.Y (y), 2.0f, 2.0f);
        }
    }
    else
    {
        for (int j = 0; j < 4; ++j)
        {
            const float cx = f.X (0.2f + 0.2f * (float) j + 0.03f * std::sin (p.t * 0.3f + (float) j));
            const float cy = f.Y (0.9f - std::fmod (hash (p.seed, j) + p.t * 0.015f, 1.0f) * 0.9f);
            const float s = f.H() * (0.05f + 0.04f * hash (p.seed, j + 9));
            const float pulse = 1.0f + 0.15f * std::sin (p.t * 2.0f + (float) j);
            glow (g, { cx, cy }, s * 2.0f, k.accent.withAlpha (0.2f));
            juce::Path bell;
            bell.addCentredArc (cx, cy, s * pulse, s * 0.8f, 0.0f, -pi * 0.5f, pi * 0.5f, true);
            bell.closeSubPath();
            g.setColour (k.light.withAlpha (0.35f));
            g.fillPath (bell);
            for (int t = 0; t < 5; ++t)
            {
                juce::Path tent;
                const float tx = cx - s * 0.7f + s * 0.35f * (float) t;
                tent.startNewSubPath (tx, cy);
                tent.quadraticTo (tx + std::sin (p.t * 1.5f + (float) t) * s * 0.3f, cy + s * 1.2f, tx, cy + s * 2.2f);
                g.setColour (k.light.withAlpha (0.25f));
                g.strokePath (tent, juce::PathStrokeType (0.8f));
            }
        }
    }
}

void snowPeaks (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.6f);
    const auto range = ridge (f, p.seed, 0.62f, 0.45f, 0.9f, 70);
    g.setColour (k.mid.withAlpha (0.8f));
    g.fillPath (range);
    {
        G::ScopedSaveState state (g);
        g.reduceClipRegion (R (f.X (0.0f), f.Y (0.0f), f.W(), f.H() * 0.42f).toNearestInt());
        g.setColour (k.light.withAlpha (0.55f));
        g.fillPath (range);
    }
    for (int i = 0; i < 4; ++i)
        softEllipse (g, { f.X (0.2f + 0.2f * (float) i) + std::fmod (p.t * 8.0f, 60.0f), f.Y (0.25f) },
                     f.W() * 0.08f, f.H() * 0.02f, k.light.withAlpha (0.25f + 0.2f * p.high));
    snow (g, f, p.seed, p.t, 100, k.light.withAlpha (0.55f));
}

void planeNight (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.8f);
    stars (g, f, p.seed, 80, 0.7f, k.light.withAlpha (0.5f), p.t);
    for (int i = 0; i < 3; ++i)
        cloud (g, f.X (0.2f + 0.3f * (float) i), f.Y (0.82f), f.W() * 0.4f, f.H() * 0.12f,
               k.mid.withAlpha (0.6f), k.light.withAlpha (0.1f), p.seed + (juce::uint32) i);

    const float x = f.X (std::fmod (p.t * 0.03f, 1.4f) - 0.2f), y = f.Y (0.35f), s = f.W() * 0.06f;
    juce::Path plane;
    plane.addRoundedRectangle (x - s * 0.5f, y - s * 0.06f, s, s * 0.12f, s * 0.06f);
    plane.addTriangle (x - s * 0.05f, y, x + s * 0.12f, y, x - s * 0.15f, y + s * 0.35f);
    plane.addTriangle (x - s * 0.5f, y - s * 0.04f, x - s * 0.38f, y - s * 0.04f, x - s * 0.5f, y - s * 0.25f);
    g.setColour (k.sil.interpolatedWith (k.light, 0.2f));
    g.fillPath (plane);
    if (std::fmod (p.t, 1.2f) < 0.1f)
        glow (g, { x, y + s * 0.2f }, s * 0.2f, k.light);
    glow (g, { x - s * 0.5f, y - s * 0.2f }, s * 0.08f, k.warm.withAlpha (0.5f + 0.5f * std::sin (p.t * 5.0f)));
}

void auroraSea (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const float horizon = 0.7f;
    nightSky (g, f, p, horizon);
    stars (g, f, p.seed, 90, 0.7f, k.light.withAlpha (0.5f), p.t);

    for (int band = 0; band < 3; ++band)
    {
        for (int i = 0; i < 70; ++i)
        {
            const float u = (float) i / 70.0f;
            const float y = f.Y (0.25f + 0.08f * (float) band) + std::sin (u * 6.0f + p.t * 0.3f + (float) band) * f.H() * 0.06f;
            const float h = f.H() * (0.15f + 0.1f * noise1 (p.seed + (juce::uint32) band, u * 8.0f + p.t * 0.2f));
            juce::ColourGradient curtain (k.accent.withAlpha (0.0f), 0.0f, y - h, k.accent.withAlpha (0.18f + 0.25f * p.mid), 0.0f, y, false);
            g.setGradientFill (curtain);
            g.fillRect (f.X (u), y - h, f.W() / 70.0f + 0.5f, h);
        }
    }

    g.setColour (k.deep.withAlpha (0.6f));
    g.fillRect (f.X (0.0f), f.Y (horizon), f.W(), f.H() * (1.0f - horizon));
    reflection (g, f.X (0.5f), f.Y (horizon) + 2.0f, f.W() * 0.5f, f.H() * 0.2f, k.accent.withAlpha (0.2f), p.t, p.seed);
}

void waves (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const float horizon = 0.4f;
    nightSky (g, f, p, horizon);
    moon (g, { f.X (0.8f), f.Y (0.15f) }, f.H() * 0.04f, k.light.withAlpha (0.8f));

    for (int i = 0; i < 16; ++i)
    {
        const float d = std::fmod ((float) i / 16.0f + p.t * 0.03f, 1.0f);
        const float y = f.Y (horizon) + f.H() * (1.0f - horizon) * d * d;
        juce::Path crest;
        for (int j = 0; j <= 40; ++j)
        {
            const float u = (float) j / 40.0f;
            const float yy = y + std::sin (u * 9.0f + p.t + (float) i) * f.H() * 0.01f * (0.3f + d);
            if (j == 0) crest.startNewSubPath (f.X (u), yy); else crest.lineTo (f.X (u), yy);
        }
        g.setColour (k.light.withAlpha (0.08f + 0.3f * d));
        g.strokePath (crest, juce::PathStrokeType (0.6f + 2.0f * d));
    }
}

void fogBank (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.5f);
    g.setColour (k.mid.withAlpha (0.6f));
    g.fillPath (ridge (f, p.seed, 0.6f, 0.2f, 0.1f, 40));
    for (int l = 0; l < 4; ++l)
        fogBand (g, f, 0.55f + 0.1f * (float) l, 0.18f, k.haze.withAlpha (0.4f + 0.1f * p.mid), p.t * 1.5f, p.seed + (juce::uint32) l * 3u);
    g.setColour (k.sil.withAlpha (0.9f));
    g.fillPath (ridge (f, p.seed + 11u, 0.9f, 0.1f, 0.0f, 40));
}

void fireflies (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.7f);
    trees (g, f, p.seed, 14, 0.72f, 0.3f, 0.45f, k.mid.withAlpha (0.5f), p.t, 0.3f);

    g.setColour (k.sil);
    g.fillRect (f.X (0.0f), f.Y (0.82f), f.W(), f.H() * 0.18f);
    for (int i = 0; i < 160; ++i)
    {
        const float x = f.X (hash (p.seed, i));
        const float h = f.H() * (0.03f + 0.06f * hash (p.seed, i + 160));
        g.drawLine (x, f.Y (0.83f), x + std::sin (p.t + (float) i) * 2.0f, f.Y (0.83f) - h, 1.0f);
    }

    for (int i = 0; i < 30; ++i)
    {
        const float x = f.X (hash (p.seed, i + 400) + 0.03f * std::sin (p.t * 0.5f + (float) i));
        const float y = f.Y (0.55f + 0.3f * hash (p.seed, i + 500) + 0.03f * std::sin (p.t * 0.7f + (float) i * 1.3f));
        const float blink = std::max (0.0f, std::sin (p.t * (1.0f + hash (p.seed, i + 600)) + (float) i));
        glow (g, { x, y }, f.H() * 0.02f, k.warm.withAlpha (blink * (0.6f + 0.4f * p.high)));
    }
}
} // namespace

void addNatureScenes (std::vector<SceneSpec>& v)
{
    //            name                           category           fn          var  ambient rhythm sparse dark intense  rare
    v.push_back ({ "Birds across the signal",    Category::Nature,  flock,      0, { 0.4f, 0.9f, 0.4f, 0.2f, 0.4f }, false });
    v.push_back ({ "Flock over the mountains",   Category::Nature,  flock,      1, { 0.6f, 0.8f, 0.4f, 0.2f, 0.3f }, false });
    v.push_back ({ "Geese in formation",         Category::Nature,  flock,      2, { 0.5f, 0.8f, 0.5f, 0.2f, 0.2f }, false });
    v.push_back ({ "Drifting storm clouds",      Category::Nature,  storm,      0, { 0.8f, 0.3f, 0.4f, 0.6f, 0.3f }, false });
    v.push_back ({ "Distant lightning",          Category::Nature,  storm,      1, { 0.4f, 0.6f, 0.2f, 0.7f, 0.9f }, false });
    v.push_back ({ "Rain and mist",              Category::Nature,  storm,      2, { 0.8f, 0.4f, 0.6f, 0.4f, 0.2f }, false });
    v.push_back ({ "Dark forest",                Category::Nature,  forest,     0, { 0.6f, 0.4f, 0.4f, 0.8f, 0.4f }, false });
    v.push_back ({ "Pine ridge",                 Category::Nature,  forest,     1, { 0.7f, 0.3f, 0.5f, 0.6f, 0.2f }, false });
    v.push_back ({ "Moonlight on the sea",       Category::Nature,  moonWater,  0, { 1.0f, 0.2f, 0.6f, 0.3f, 0.0f }, false });
    v.push_back ({ "Moon over the reeds",        Category::Nature,  moonWater,  1, { 0.9f, 0.2f, 0.7f, 0.3f, 0.0f }, false });
    v.push_back ({ "Underwater currents",        Category::Nature,  underwater, 0, { 1.0f, 0.3f, 0.4f, 0.5f, 0.1f }, false });
    v.push_back ({ "Drifting jellyfish",         Category::Nature,  underwater, 1, { 1.0f, 0.2f, 0.4f, 0.5f, 0.1f }, true  });
    v.push_back ({ "Snow across the peaks",      Category::Nature,  snowPeaks,  0, { 0.9f, 0.3f, 0.5f, 0.4f, 0.2f }, false });
    v.push_back ({ "Night flight",               Category::Nature,  planeNight, 0, { 0.6f, 0.6f, 0.5f, 0.2f, 0.2f }, false });
    v.push_back ({ "Aurora over the sea",        Category::Nature,  auroraSea,  0, { 1.0f, 0.3f, 0.4f, 0.3f, 0.2f }, false });
    v.push_back ({ "Night waves",                Category::Nature,  waves,      0, { 0.8f, 0.6f, 0.4f, 0.3f, 0.3f }, false });
    v.push_back ({ "Fog bank",                   Category::Nature,  fogBank,    0, { 1.0f, 0.1f, 0.5f, 0.5f, 0.0f }, false });
    v.push_back ({ "Fireflies in the meadow",    Category::Nature,  fireflies,  0, { 0.8f, 0.3f, 0.8f, 0.2f, 0.0f }, false });
}
}

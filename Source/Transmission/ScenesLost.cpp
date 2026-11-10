// =================================================================================================
//  LOST WORLDS & ANCIENT CIVILISATIONS
// =================================================================================================
#include "SceneKit.h"
#include "../World/PixelArt.h"
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
    sky (g, f, k.deep.withAlpha (0.0f), k.haze.withAlpha (0.55f), k.deep.withAlpha (0.5f), horizon);
}

// ------------------------------------------------------------------ underwater
void underwaterBase (G& g, const Frame& f, const P& p)
{
    const auto& k = p.ink;
    sky (g, f, k.haze.withAlpha (0.5f), k.deep.withAlpha (0.55f), k.deep.withAlpha (0.8f), 0.35f);

    for (int i = 0; i < 5; ++i)
    {
        const float x = f.X (0.1f + 0.2f * (float) i + 0.03f * std::sin (p.t * 0.3f + (float) i));
        const float w = f.W() * (0.02f + 0.03f * hash (p.seed, i));
        juce::Path shaft;
        shaft.startNewSubPath (x - w, f.Y (0.0f));
        shaft.lineTo (x + w, f.Y (0.0f));
        shaft.lineTo (x + w * 3.0f + f.W() * 0.08f, f.Y (1.0f));
        shaft.lineTo (x + w + f.W() * 0.08f, f.Y (1.0f));
        shaft.closeSubPath();
        juce::ColourGradient sg (k.light.withAlpha (0.10f + 0.06f * p.mid), 0.0f, f.Y (0.0f), k.light.withAlpha (0.0f), 0.0f, f.Y (0.9f), false);
        g.setGradientFill (sg);
        g.fillPath (shaft);
    }

    for (int i = 0; i < 40; ++i)
    {
        const float bx = f.X (hash (p.seed, 100 + i) + 0.02f * std::sin (p.t * 0.7f + (float) i));
        const float by = f.Y (1.0f - std::fmod (hash (p.seed, 200 + i) + p.t * (0.01f + 0.03f * hash (p.seed, 300 + i)), 1.0f));
        const float s = 1.0f + 2.5f * hash (p.seed, 400 + i);
        g.setColour (k.light.withAlpha (0.2f + 0.3f * p.high));
        g.drawEllipse (bx, by, s, s, 0.7f);
    }
}

void sunkenCity (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    underwaterBase (g, f, p);

    for (int row = 0; row < 2; ++row)
    {
        const auto col = row == 0 ? k.mid.withAlpha (0.4f) : k.sil.withAlpha (0.9f);
        const int n = row == 0 ? 16 : 10;

        for (int i = 0; i < n; ++i)
        {
            const int s = i + row * 50;
            const float u = ((float) i + hash (p.seed, s)) / (float) n;
            const float w = f.W() * (row == 0 ? 0.035f : 0.06f) * (0.7f + 0.6f * hash (p.seed, s + 70));
            const float h = f.H() * (row == 0 ? 0.25f : 0.38f) * (0.4f + hash (p.seed, s + 90));
            const float base = f.Y (row == 0 ? 0.86f : 1.02f);
            const float tilt = (hash (p.seed, s + 130) - 0.5f) * 0.25f;
            const auto tr = juce::AffineTransform::rotation (tilt).translated (f.X (u), base);

            juce::Path b;
            b.addRectangle (-w * 0.5f, -h, w, h);
            if (hash (p.seed, s + 150) > 0.6f)
                b.addTriangle (-w * 0.6f, -h, w * 0.6f, -h, 0.0f, -h - w * 0.8f);
            b.applyTransform (tr);
            g.setColour (col);
            g.fillPath (b);

            if (row == 1)
            {
                for (int j = 0; j < 6; ++j)
                {
                    if (hash (p.seed, s * 7 + j) < 0.75f)
                        continue;
                    const auto wp = juce::Point<float> ((hash (p.seed, s * 9 + j) - 0.5f) * w * 0.7f, -h * (0.15f + 0.7f * hash (p.seed, s * 11 + j))).transformedBy (tr);
                    g.setColour (k.accent.withAlpha (0.35f + 0.3f * p.mid));
                    g.fillRect (wp.x, wp.y, w * 0.12f, w * 0.12f);
                }
            }
        }
    }
}

void drownedTemple (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    underwaterBase (g, f, p);

    const float cx = f.X (0.5f), base = f.Y (0.9f), w = f.W() * 0.42f, colH = f.H() * 0.32f;
    g.setColour (k.sil.withAlpha (0.9f));
    for (int s = 0; s < 3; ++s)
        g.fillRect (cx - w * 0.5f - (float) s * w * 0.04f, base + (float) s * f.H() * 0.025f, w + (float) s * w * 0.08f, f.H() * 0.025f);

    for (int i = 0; i < 6; ++i)
    {
        const float x = cx - w * 0.45f + w * 0.9f * (float) i / 5.0f;
        const float h = (i == 4 ? 0.55f : 1.0f) * colH;
        g.fillRect (x - w * 0.03f, base - h, w * 0.06f, h);
        g.setColour (k.light.withAlpha (0.15f));
        g.fillRect (x - w * 0.03f, base - h, w * 0.012f, h);
        g.setColour (k.sil.withAlpha (0.9f));
    }

    g.fillRect (cx - w * 0.52f, base - colH - f.H() * 0.04f, w * 0.75f, f.H() * 0.04f);
    juce::Path ped;
    ped.addTriangle (cx - w * 0.52f, base - colH - f.H() * 0.04f, cx + w * 0.22f, base - colH - f.H() * 0.04f, cx - w * 0.15f, base - colH - f.H() * 0.14f);
    g.fillPath (ped);

    for (int i = 0; i < 5; ++i)
    {
        const float fx = std::fmod (hash (p.seed, i + 40) + p.t * 0.02f * (1.0f + hash (p.seed, i)), 1.2f) - 0.1f;
        const float fy = 0.3f + 0.4f * hash (p.seed, i + 60);
        g.setColour (k.mid.withAlpha (0.5f));
        g.fillEllipse (f.X (fx), f.Y (fy), f.W() * 0.025f, f.H() * 0.012f);
    }
}

void columnsDeep (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    underwaterBase (g, f, p);

    for (int d = 5; d >= 0; --d)
    {
        const float s = 1.0f / (1.0f + (float) d * 0.55f);
        const auto col = k.mid.interpolatedWith (k.sil, 1.0f - (float) d / 6.0f).withAlpha (0.4f + 0.5f * s);
        const float base = f.Y (0.55f + 0.45f * s);
        for (int side = -1; side <= 1; side += 2)
        {
            const float x = f.X (0.5f + (float) side * 0.38f * s);
            const float h = f.H() * 0.75f * s * (hash (p.seed, d * 2 + side + 3) > 0.7f ? 0.5f : 1.0f);
            const float w = f.W() * 0.05f * s;
            g.setColour (col);
            g.fillRect (x - w * 0.5f, base - h, w, h);
            g.fillRect (x - w * 0.7f, base - h - w * 0.3f, w * 1.4f, w * 0.3f);
        }
    }
}

// ------------------------------------------------------------------ mountains & fog
void fogMountains (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    sky (g, f, k.deep.withAlpha (0.1f), k.haze.withAlpha (0.65f), k.deep.withAlpha (0.5f), 0.6f);

    if (p.variant == 2)
        moon (g, { f.X (0.72f), f.Y (0.22f) }, f.H() * 0.05f, k.light.withAlpha (0.8f));

    const int layers = p.variant == 2 ? 4 : 3;
    for (int l = 0; l < layers; ++l)
    {
        const float d = (float) (l + 1) / (float) layers;
        const auto col = k.haze.interpolatedWith (k.sil, d).withAlpha (0.85f);

        if (p.variant == 1 && l == layers - 2)
        {
            juce::Path twin;
            twin.startNewSubPath (f.X (0.0f), f.Y (1.0f));
            twin.lineTo (f.X (0.0f), f.Y (0.75f));
            twin.quadraticTo (f.X (0.2f), f.Y (0.7f), f.X (0.32f), f.Y (0.22f));
            twin.quadraticTo (f.X (0.42f), f.Y (0.55f), f.X (0.52f), f.Y (0.5f));
            twin.quadraticTo (f.X (0.6f), f.Y (0.45f), f.X (0.68f), f.Y (0.18f));
            twin.quadraticTo (f.X (0.8f), f.Y (0.68f), f.X (1.0f), f.Y (0.72f));
            twin.lineTo (f.X (1.0f), f.Y (1.0f));
            twin.closeSubPath();
            g.setColour (col);
            g.fillPath (twin);
        }
        else
        {
            g.setColour (col);
            g.fillPath (ridge (f, p.seed + (juce::uint32) l * 7u, 0.5f + 0.14f * (float) l, 0.3f - 0.05f * (float) l, 0.6f, 60));
        }

        fogBand (g, f, 0.6f + 0.12f * (float) l, 0.12f, k.haze.withAlpha (0.3f + 0.2f * p.mid), p.t, p.seed + (juce::uint32) l);
    }
}

void forestRuins (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.7f);
    trees (g, f, p.seed, 18, 0.85f, 0.35f, 0.6f, k.mid.withAlpha (0.5f), p.t, 0.4f);

    // shaft of light onto the ruin
    juce::Path shaft;
    shaft.startNewSubPath (f.X (0.42f), f.Y (0.0f));
    shaft.lineTo (f.X (0.52f), f.Y (0.0f));
    shaft.lineTo (f.X (0.62f), f.Y (0.9f));
    shaft.lineTo (f.X (0.4f), f.Y (0.9f));
    shaft.closeSubPath();
    juce::ColourGradient sg (k.light.withAlpha (0.18f), 0.0f, f.Y (0.0f), k.light.withAlpha (0.02f), 0.0f, f.Y (0.9f), false);
    g.setGradientFill (sg);
    g.fillPath (shaft);

    // arch with an opening (even-odd fill)
    const float cx = f.X (0.5f), base = f.Y (0.9f), aw = f.W() * 0.16f, ah = f.H() * 0.45f;
    juce::Path arch;
    arch.addRectangle (cx - aw * 0.5f, base - ah, aw, ah);
    arch.startNewSubPath (cx - aw * 0.3f, base);
    arch.lineTo (cx - aw * 0.3f, base - ah * 0.55f);
    arch.addCentredArc (cx, base - ah * 0.55f, aw * 0.3f, aw * 0.3f, 0.0f, -pi * 0.5f, pi * 0.5f, false);
    arch.lineTo (cx + aw * 0.3f, base);
    arch.closeSubPath();
    arch.setUsingNonZeroWinding (false);
    g.setColour (k.sil.withAlpha (0.92f));
    g.fillPath (arch);

    // broken wall
    juce::Path wall;
    wall.startNewSubPath (cx + aw * 0.6f, base);
    wall.lineTo (cx + aw * 0.6f, base - ah * 0.35f);
    wall.lineTo (cx + aw * 0.9f, base - ah * 0.35f);
    wall.lineTo (cx + aw * 0.9f, base - ah * 0.2f);
    wall.lineTo (cx + aw * 1.3f, base - ah * 0.2f);
    wall.lineTo (cx + aw * 1.3f, base);
    wall.closeSubPath();
    g.fillPath (wall);

    trees (g, f, p.seed + 3u, 6, 1.0f, 0.6f, 0.95f, k.sil.withAlpha (0.95f), p.t, 0.7f);
}

void staircase (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, p.variant == 0 ? 0.85f : 0.5f);
    stars (g, f, p.seed, 50, 0.6f, k.light.withAlpha (0.6f), p.t);

    const int n = 14;
    const float sw = f.W() * 0.035f, sh = f.H() * 0.045f;
    const float x0 = f.X (0.25f), y0 = f.Y (0.9f);
    const juce::Point<float> top (x0 + sw * (float) n, y0 - sh * (float) n);

    glow (g, top, f.H() * 0.25f, k.warm.withAlpha (0.4f + 0.3f * p.mid));
    g.setColour (k.light.withAlpha (0.8f));
    g.fillRect (top.x, top.y - sh * 3.0f, sw * 1.4f, sh * 3.0f);

    for (int i = 0; i < n; ++i)
    {
        const float x = x0 + sw * (float) i, y = y0 - sh * (float) i;
        g.setColour (k.sil.withAlpha (0.9f));
        if (p.variant == 0)
            g.fillRect (x, y - sh, sw * 1.02f, f.Y (1.0f) - y + sh);
        else
        {
            const float bob = std::sin (p.t * 0.8f + (float) i * 0.5f) * sh * 0.15f;
            g.fillRect (x, y - sh + bob, sw * 1.02f, sh * 0.35f);
        }
        g.setColour (k.light.withAlpha (0.18f));
        g.fillRect (x, y - sh, sw * 1.02f, 1.0f);
    }

    const float climb = std::fmod (p.t * 0.25f, 1.0f) * (float) n;
    figure (g, x0 + sw * climb, y0 - sh * climb - sh, f.H() * 0.09f, k.sil, p.t * 4.0f, true);
}

void doorway (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const float horizon = 0.66f;
    nightSky (g, f, p, horizon);
    stars (g, f, p.seed, 60, 0.6f, k.light.withAlpha (0.5f), p.t);

    if (p.variant == 0)
    {
        g.setColour (k.mid.withAlpha (0.6f));
        g.fillPath (ridge (f, p.seed, 0.75f, 0.08f, 0.0f, 40));
        g.setColour (k.sil.withAlpha (0.85f));
        g.fillPath (ridge (f, p.seed + 9u, 0.85f, 0.06f, 0.0f, 40));
    }
    else if (p.variant == 1)
    {
        g.setColour (k.deep.withAlpha (0.7f));
        g.fillRect (f.X (0.0f), f.Y (horizon), f.W(), f.H() * (1.0f - horizon));
    }
    else
    {
        trees (g, f, p.seed, 20, 0.85f, 0.4f, 0.75f, k.sil.withAlpha (0.85f), p.t, 0.4f);
    }

    const float dw = f.W() * 0.07f, dh = f.H() * 0.3f, cx = f.X (0.5f), base = f.Y (p.variant == 1 ? horizon + 0.04f : 0.86f);
    const float light = 0.6f + 0.4f * p.mid;

    juce::Path spill;
    spill.startNewSubPath (cx - dw * 0.5f, base);
    spill.lineTo (cx + dw * 0.5f, base);
    spill.lineTo (cx + dw * 2.2f, f.Y (1.0f));
    spill.lineTo (cx - dw * 2.2f, f.Y (1.0f));
    spill.closeSubPath();
    juce::ColourGradient sg (k.warm.withAlpha (0.35f * light), cx, base, k.warm.withAlpha (0.0f), cx, f.Y (1.0f), false);
    g.setGradientFill (sg);
    g.fillPath (spill);

    glow (g, { cx, base - dh * 0.5f }, dh, k.warm.withAlpha (0.25f * light));
    g.setColour (k.light.interpolatedWith (k.warm, 0.4f).withAlpha (0.9f));
    g.fillRect (cx - dw * 0.5f, base - dh, dw, dh);
    g.setColour (k.sil);
    g.drawRect (cx - dw * 0.5f - 2.0f, base - dh - 2.0f, dw + 4.0f, dh + 2.0f, 3.0f);

    if (p.variant == 1)
        reflection (g, cx, base + 2.0f, dw * 1.3f, f.H() * 0.25f, k.warm.withAlpha (0.4f), p.t, p.seed);
}

void pyramids (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.75f);
    stars (g, f, p.seed, 90, 0.7f, k.light.withAlpha (0.6f), p.t);

    if (p.variant == 1)
    {
        // three aligned stars and the line that points at them
        for (int i = 0; i < 3; ++i)
        {
            const juce::Point<float> s (f.X (0.58f + 0.05f * (float) i), f.Y (0.18f + 0.025f * (float) i));
            glow (g, s, f.H() * 0.03f, k.light.withAlpha (0.7f));
        }
        g.setColour (k.accent.withAlpha (0.25f + 0.3f * p.bass));
        g.drawLine (f.X (0.42f), f.Y (0.48f), f.X (0.63f), f.Y (0.205f), 1.0f);
    }

    g.setColour (k.mid.withAlpha (0.6f));
    g.fillPath (ridge (f, p.seed, 0.86f, 0.05f, 0.0f, 30));

    const float sizes[] = { 0.40f, 0.30f, 0.18f };
    const float xs[] = { 0.42f, 0.66f, 0.82f };
    for (int i = 0; i < 3; ++i)
    {
        const float h = f.H() * sizes[i], cx = f.X (xs[i]), base = f.Y (0.88f);
        juce::Path left, right;
        left.addTriangle (cx - h * 0.8f, base, cx, base, cx, base - h);
        right.addTriangle (cx, base, cx + h * 0.8f, base, cx, base - h);
        g.setColour (k.mid.interpolatedWith (k.light, 0.25f + 0.3f * p.bass).withAlpha (0.85f));
        g.fillPath (left);
        g.setColour (k.sil.withAlpha (0.95f));
        g.fillPath (right);
    }
}

void stoneCircle (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.6f);
    glow (g, { f.X (0.5f), f.Y (0.6f) }, f.H() * 0.45f, k.warm.withAlpha (0.35f + 0.2f * p.mid));
    g.setColour (k.sil.withAlpha (0.8f));
    g.fillRect (f.X (0.0f), f.Y (0.6f), f.W(), f.H() * 0.4f);

    struct Stone { float x, y, s; };
    std::vector<Stone> stones;
    for (int i = 0; i < 12; ++i)
    {
        const float a = (float) i / 12.0f * 2.0f * pi + 0.2f;
        stones.push_back ({ 0.5f + 0.34f * std::sin (a), 0.72f - 0.1f * std::cos (a), 0.0f });
    }
    std::sort (stones.begin(), stones.end(), [] (const Stone& a, const Stone& b) { return a.y < b.y; });

    for (size_t i = 0; i < stones.size(); ++i)
    {
        const auto& s = stones[i];
        const float scale = 0.6f + (s.y - 0.62f) * 2.0f;
        const float h = f.H() * 0.2f * scale * (0.8f + 0.4f * hash (p.seed, (int) i));
        const float w = h * 0.38f;
        g.setColour (k.sil.interpolatedWith (k.mid, 0.2f * (1.0f - scale)).withAlpha (0.95f));
        g.fillRoundedRectangle (f.X (s.x) - w * 0.5f, f.Y (s.y) - h, w, h, w * 0.25f);
    }

    g.setColour (k.warm.withAlpha (0.2f + 0.3f * p.bass));
    g.drawLine (f.X (0.5f), f.Y (0.6f), f.X (0.5f), f.Y (1.0f), 1.5f);
}

void runestone (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.65f);

    if (p.variant == 1)
    {
        g.setColour (k.mid.withAlpha (0.7f));
        g.fillPath (ridge (f, p.seed, 0.55f, 0.35f, 0.8f, 50));
        g.setColour (k.deep.withAlpha (0.6f));
        g.fillRect (f.X (0.0f), f.Y (0.7f), f.W(), f.H() * 0.3f);
    }
    else
    {
        g.setColour (k.light.withAlpha (0.25f));
        g.fillRect (f.X (0.0f), f.Y (0.82f), f.W(), f.H() * 0.18f);
    }

    const float cx = f.X (0.5f), base = f.Y (0.88f), w = f.W() * 0.13f, h = f.H() * 0.55f;
    juce::Path stone;
    stone.startNewSubPath (cx - w * 0.5f, base);
    stone.lineTo (cx - w * 0.55f, base - h * 0.7f);
    stone.quadraticTo (cx - w * 0.45f, base - h, cx + w * 0.1f, base - h);
    stone.quadraticTo (cx + w * 0.55f, base - h * 0.85f, cx + w * 0.5f, base);
    stone.closeSubPath();
    g.setColour (k.sil.withAlpha (0.95f));
    g.fillPath (stone);

    const float glowAmt = 0.35f + 0.65f * p.bass;
    const int runes[] = { (int) (hash (p.seed, 1) * 24.0f), (int) (hash (p.seed, 2) * 24.0f), (int) (hash (p.seed, 3) * 24.0f) };
    for (int i = 0; i < 3; ++i)
    {
        const auto box = R (w * 0.28f, h * 0.18f).withCentre ({ cx, base - h * (0.22f + 0.24f * (float) i) });
        pixel::drawRune (g, runes[i], box, k.accent, glowAmt, juce::jmax (1.0f, w * 0.03f));
    }

    snow (g, f, p.seed, p.t, 80, k.light.withAlpha (0.6f));
}

void torii (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const float water = 0.72f;
    nightSky (g, f, p, water);

    auto drawGate = [&] (float sy, float alpha)
    {
        const float cx = f.X (0.5f), base = f.Y (water), w = f.W() * 0.28f, h = f.H() * 0.42f * sy;
        g.setColour (k.sil.withAlpha (alpha));
        g.fillRect (cx - w * 0.36f, base - h, w * 0.06f, h);
        g.fillRect (cx + w * 0.30f, base - h, w * 0.06f, h);
        g.fillRect (cx - w * 0.45f, base - h * 0.78f, w * 0.9f, h * 0.05f);
        juce::Path top;
        top.startNewSubPath (cx - w * 0.6f, base - h * 1.02f);
        top.quadraticTo (cx, base - h * 0.9f, cx + w * 0.6f, base - h * 1.02f);
        top.lineTo (cx + w * 0.55f, base - h * 0.92f);
        top.quadraticTo (cx, base - h * 0.84f, cx - w * 0.55f, base - h * 0.92f);
        top.closeSubPath();
        g.fillPath (top);
    };

    if (p.variant == 1)
    {
        g.setColour (k.mid.withAlpha (0.7f));
        g.fillPath (ridge (f, p.seed, 0.8f, 0.15f, 0.0f, 30));
        for (int i = 0; i < 6; ++i)
            glow (g, { f.X (0.15f + 0.12f * (float) i), f.Y (0.86f - 0.02f * (float) i) }, f.H() * 0.03f, k.warm.withAlpha (0.8f));
        drawGate (1.0f, 0.95f);
    }
    else
    {
        g.setColour (k.deep.withAlpha (0.5f));
        g.fillRect (f.X (0.0f), f.Y (water), f.W(), f.H() * (1.0f - water));
        drawGate (1.0f, 0.95f);
        {
            G::ScopedSaveState state (g);
            g.addTransform (juce::AffineTransform::scale (1.0f, -0.7f, 0.0f, f.Y (water)));
            drawGate (1.0f, 0.3f);
        }
    }

    fogBand (g, f, water - 0.05f, 0.12f, k.haze.withAlpha (0.35f), p.t, p.seed);
}

void pagoda (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.6f);
    moon (g, { f.X (0.2f), f.Y (0.2f) }, f.H() * 0.05f, k.light.withAlpha (0.7f));

    const float peakY = p.variant == 0 ? 0.55f : 0.62f;
    juce::Path mountain;
    mountain.startNewSubPath (f.X (0.15f), f.Y (1.0f));
    mountain.quadraticTo (f.X (0.4f), f.Y (peakY + 0.05f), f.X (0.5f), f.Y (peakY));
    mountain.quadraticTo (f.X (0.6f), f.Y (peakY + 0.05f), f.X (0.85f), f.Y (1.0f));
    mountain.closeSubPath();
    g.setColour (k.sil.withAlpha (0.9f));
    g.fillPath (mountain);

    const float cx = f.X (0.5f);
    float y = f.Y (peakY);
    for (int t = 0; t < 5; ++t)
    {
        const float w = f.W() * (0.11f - 0.016f * (float) t), h = f.H() * 0.055f;
        g.fillRect (cx - w * 0.35f, y - h, w * 0.7f, h);
        juce::Path roof;
        roof.startNewSubPath (cx - w * 0.75f, y - h - h * 0.15f);
        roof.quadraticTo (cx - w * 0.4f, y - h + h * 0.05f, cx, y - h - h * 0.45f);
        roof.quadraticTo (cx + w * 0.4f, y - h + h * 0.05f, cx + w * 0.75f, y - h - h * 0.15f);
        roof.lineTo (cx, y - h - h * 0.2f);
        roof.closeSubPath();
        g.fillPath (roof);
        if (hash (p.seed, t) > 0.5f)
            glow (g, { cx, y - h * 0.5f }, h * 0.6f, k.warm.withAlpha (0.6f));
        y -= h * 1.35f;
    }
    g.drawLine (cx, y, cx, y - f.H() * 0.06f, 1.5f);

    if (p.variant == 1)
        for (int i = 0; i < 3; ++i)
            fogBand (g, f, 0.6f + 0.12f * (float) i, 0.12f, k.haze.withAlpha (0.4f), p.t, p.seed + (juce::uint32) i);
}

void buriedStatue (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.7f);
    stars (g, f, p.seed, 60, 0.5f, k.light.withAlpha (0.5f), p.t);

    // giant stone face in profile, half buried
    const float s = f.H() * 0.5f, x = f.X (0.45f), y = f.Y (0.8f);
    juce::Path head;
    head.startNewSubPath (x - s * 0.5f, y);
    head.cubicTo (x - s * 0.6f, y - s * 0.7f, x - s * 0.1f, y - s * 1.05f, x + s * 0.25f, y - s * 0.9f);
    head.lineTo (x + s * 0.28f, y - s * 0.62f);
    head.lineTo (x + s * 0.42f, y - s * 0.45f);
    head.lineTo (x + s * 0.3f, y - s * 0.42f);
    head.lineTo (x + s * 0.33f, y - s * 0.3f);
    head.lineTo (x + s * 0.3f, y - s * 0.22f);
    head.lineTo (x + s * 0.32f, y);
    head.closeSubPath();
    g.setColour (k.mid.interpolatedWith (k.sil, 0.5f).withAlpha (0.95f));
    g.fillPath (head);
    g.setColour (k.sil.withAlpha (0.9f));
    g.fillEllipse (x + s * 0.05f, y - s * 0.68f, s * 0.12f, s * 0.05f);

    g.setColour (k.sil.withAlpha (0.95f));
    g.fillPath (ridge (f, p.seed, 0.82f, 0.05f, 0.0f, 30));

    // excavation strings and tiny archaeologists with lanterns
    g.setColour (k.light.withAlpha (0.25f));
    for (int i = 0; i < 6; ++i)
        g.drawLine (f.X (0.2f + 0.1f * (float) i), f.Y (0.8f), f.X (0.25f + 0.1f * (float) i), f.Y (0.95f), 0.6f);
    for (int i = 0; i < 3; ++i)
    {
        const float fx = f.X (0.68f + 0.07f * (float) i);
        figure (g, fx, f.Y (0.84f), f.H() * 0.07f, k.sil, 0.0f, false);
        glow (g, { fx + f.H() * 0.02f, f.Y (0.81f) }, f.H() * 0.04f, k.warm.withAlpha (0.7f));
    }
}

void ancientMap (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const auto ink = k.light.withAlpha (0.55f);
    g.setColour (k.haze.withAlpha (0.25f));
    g.fillRect (f.r.reduced (f.W() * 0.08f, f.H() * 0.08f));
    g.setColour (ink);
    g.drawRect (f.r.reduced (f.W() * 0.08f, f.H() * 0.08f), 1.0f);

    const juce::Point<float> rose (f.X (0.62f), f.Y (0.5f));

    if (p.variant == 0)
    {
        // portolan-style chart: wandering coast and rhumb lines
        juce::Path coast;
        for (int i = 0; i <= 60; ++i)
        {
            const float u = (float) i / 60.0f;
            const float x = f.X (0.12f + 0.3f * noise1 (p.seed, u * 6.0f) + 0.05f * std::sin (u * 20.0f));
            const float y = f.Y (0.12f + 0.76f * u);
            if (i == 0) coast.startNewSubPath (x, y); else coast.lineTo (x, y);
        }
        g.strokePath (coast, juce::PathStrokeType (1.2f));

        g.setColour (ink.withMultipliedAlpha (0.4f));
        for (int i = 0; i < 16; ++i)
        {
            const float a = (float) i / 16.0f * 2.0f * pi;
            g.drawLine (rose.x, rose.y, rose.x + std::sin (a) * f.W(), rose.y - std::cos (a) * f.W(), 0.5f);
        }

        juce::Path star;
        for (int i = 0; i < 16; ++i)
        {
            const float a = (float) i / 16.0f * 2.0f * pi + p.t * 0.05f;
            const float r = (i % 2 == 0 ? 0.14f : 0.04f) * f.H();
            const juce::Point<float> v (rose.x + std::sin (a) * r, rose.y - std::cos (a) * r);
            if (i == 0) star.startNewSubPath (v); else star.lineTo (v);
        }
        star.closeSubPath();
        glowStroke (g, star, k.accent, 1.0f, 0.5f + 0.4f * p.mid);
    }
    else
    {
        // star chart: circles of declination, ecliptic, dots
        for (int i = 1; i <= 3; ++i)
        {
            const float r = f.H() * 0.12f * (float) i;
            g.setColour (ink.withMultipliedAlpha (0.5f));
            g.drawEllipse (rose.x - r, rose.y - r, r * 2.0f, r * 2.0f, 0.7f);
        }
        juce::Path ecl;
        ecl.addCentredArc (rose.x, rose.y + f.H() * 0.1f, f.H() * 0.3f, f.H() * 0.22f, 0.3f, 0.0f, 2.0f * pi, true);
        glowStroke (g, ecl, k.accent, 1.0f, 0.4f + 0.4f * p.mid);
        stars (g, Frame { R (rose.x - f.H() * 0.36f, rose.y - f.H() * 0.36f, f.H() * 0.72f, f.H() * 0.72f) }, p.seed, 40, 1.0f, k.light, p.t);
    }
}

void ziggurat (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.78f);
    stars (g, f, p.seed, 70, 0.6f, k.light.withAlpha (0.6f), p.t);
    moon (g, { f.X (0.78f), f.Y (0.2f) }, f.H() * 0.04f, k.light.withAlpha (0.8f));

    const float cx = f.X (0.45f);
    float y = f.Y (0.88f);
    for (int t = 0; t < 4; ++t)
    {
        const float w = f.W() * (0.5f - 0.1f * (float) t), h = f.H() * 0.09f;
        g.setColour (k.mid.interpolatedWith (k.sil, 0.6f).withAlpha (0.95f));
        g.fillRect (cx - w * 0.5f, y - h, w, h);
        g.setColour (k.light.withAlpha (0.15f + 0.25f * p.bass));
        g.fillRect (cx - w * 0.5f, y - h, w, 1.0f);
        y -= h;
    }
    g.setColour (k.sil.withAlpha (0.9f));
    juce::Path stairs;
    stairs.addTriangle (cx - f.W() * 0.04f, f.Y (0.88f), cx + f.W() * 0.04f, f.Y (0.88f), cx, y);
    g.fillPath (stairs);
    glow (g, { cx, y - f.H() * 0.02f }, f.H() * 0.06f, k.warm.withAlpha (0.6f));
}

void floatingIsland (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.5f);
    stars (g, f, p.seed, 50, 0.9f, k.light.withAlpha (0.5f), p.t);

    const float bob = std::sin (p.t * 0.5f) * f.H() * 0.02f;
    const float cx = f.X (0.5f), top = f.Y (0.45f) + bob, w = f.W() * 0.36f;
    juce::Path rock;
    rock.startNewSubPath (cx - w * 0.5f, top);
    rock.lineTo (cx + w * 0.5f, top);
    rock.quadraticTo (cx + w * 0.3f, top + f.H() * 0.15f, cx + w * 0.05f, top + f.H() * 0.35f);
    rock.quadraticTo (cx - w * 0.2f, top + f.H() * 0.18f, cx - w * 0.5f, top);
    rock.closeSubPath();
    g.setColour (k.sil.withAlpha (0.95f));
    g.fillPath (rock);

    for (int i = 0; i < 4; ++i)
    {
        const float x = cx - w * 0.3f + w * 0.18f * (float) i;
        const float h = f.H() * (i == 2 ? 0.06f : 0.12f);
        g.fillRect (x, top - h, w * 0.04f, h);
    }
    g.fillRect (cx - w * 0.32f, top - f.H() * 0.13f, w * 0.38f, f.H() * 0.015f);

    g.setColour (k.light.withAlpha (0.3f + 0.2f * p.high));
    for (int i = 0; i < 3; ++i)
    {
        const float x = cx + w * (0.35f + 0.04f * (float) i);
        g.drawLine (x, top + 2.0f, x + 2.0f, f.Y (1.0f), 0.8f);
    }
    for (int i = 0; i < 4; ++i)
        bird (g, f.X (std::fmod (hash (p.seed, i) + p.t * 0.03f, 1.0f)), f.Y (0.2f + 0.1f * hash (p.seed, i + 9)),
              f.H() * 0.02f, p.t * 5.0f + (float) i, k.sil);
}

void obelisk (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    sky (g, f, k.deep.withAlpha (0.0f), k.warm.withAlpha (0.35f), k.deep.withAlpha (0.6f), 0.72f);
    g.setColour (k.sil.withAlpha (0.85f));
    g.fillRect (f.X (0.0f), f.Y (0.78f), f.W(), f.H() * 0.22f);

    const int n = p.variant == 1 ? 2 : 1;
    for (int i = 0; i < n; ++i)
    {
        const float cx = f.X (n == 1 ? 0.5f : 0.4f + 0.2f * (float) i), base = f.Y (0.8f);
        const float w = f.W() * 0.035f, h = f.H() * 0.55f;

        juce::Path shadow;
        shadow.startNewSubPath (cx - w * 0.5f, base);
        shadow.lineTo (cx + w * 0.5f, base);
        shadow.lineTo (cx + w * 0.2f + f.W() * 0.35f, f.Y (0.98f));
        shadow.lineTo (cx - w * 0.2f + f.W() * 0.33f, f.Y (0.98f));
        shadow.closeSubPath();
        g.setColour (k.deep.withAlpha (0.5f));
        g.fillPath (shadow);

        juce::Path o;
        o.startNewSubPath (cx - w * 0.5f, base);
        o.lineTo (cx - w * 0.35f, base - h);
        o.lineTo (cx, base - h - w * 0.6f);
        o.lineTo (cx + w * 0.35f, base - h);
        o.lineTo (cx + w * 0.5f, base);
        o.closeSubPath();
        g.setColour (k.sil.withAlpha (0.95f));
        g.fillPath (o);
        glow (g, { cx, base - h - w * 0.3f }, w * 1.5f, k.warm.withAlpha (0.4f + 0.4f * p.bass));
    }
}

void longhouse (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    nightSky (g, f, p, 0.6f);
    g.setColour (k.mid.withAlpha (0.6f));
    g.fillPath (ridge (f, p.seed, 0.55f, 0.3f, 0.7f, 50));
    g.setColour (k.light.withAlpha (0.25f));
    g.fillRect (f.X (0.0f), f.Y (0.78f), f.W(), f.H() * 0.22f);

    const float cx = f.X (0.5f), base = f.Y (0.8f), w = f.W() * 0.36f, h = f.H() * 0.16f;
    juce::Path house;
    house.startNewSubPath (cx - w * 0.5f, base);
    house.lineTo (cx - w * 0.52f, base - h * 0.5f);
    house.quadraticTo (cx, base - h * 1.25f, cx + w * 0.52f, base - h * 0.5f);
    house.lineTo (cx + w * 0.5f, base);
    house.closeSubPath();
    g.setColour (k.sil.withAlpha (0.95f));
    g.fillPath (house);

    glow (g, { cx, base - h * 0.2f }, h * 0.8f, k.warm.withAlpha (0.5f + 0.3f * p.mid));
    g.setColour (k.warm.withAlpha (0.9f));
    g.fillRect (cx - w * 0.03f, base - h * 0.4f, w * 0.06f, h * 0.4f);

    for (int i = 0; i < 6; ++i)
    {
        const float rise = std::fmod (p.t * 0.08f + (float) i / 6.0f, 1.0f);
        softEllipse (g, { cx + w * 0.2f + rise * f.W() * 0.06f, base - h * 1.1f - rise * f.H() * 0.4f },
                     f.H() * (0.02f + 0.05f * rise), f.H() * (0.015f + 0.03f * rise), k.haze.withAlpha (0.4f * (1.0f - rise)));
    }
    snow (g, f, p.seed, p.t, 90, k.light.withAlpha (0.6f));
}

void lighthouse (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const float horizon = 0.7f;
    nightSky (g, f, p, horizon);
    g.setColour (k.deep.withAlpha (0.6f));
    g.fillRect (f.X (0.0f), f.Y (horizon), f.W(), f.H() * (1.0f - horizon));

    const float cx = f.X (0.3f), base = f.Y (horizon + 0.05f), h = f.H() * 0.4f;
    const juce::Point<float> lamp (cx, base - h);
    const float a = p.t * 0.9f;

    for (int side = 0; side < 2; ++side)
    {
        const float dir = std::sin (a + (float) side * pi);
        juce::Path beam;
        beam.startNewSubPath (lamp);
        beam.lineTo (lamp.x + dir * f.W() * 0.8f, lamp.y - f.H() * 0.06f);
        beam.lineTo (lamp.x + dir * f.W() * 0.8f, lamp.y + f.H() * 0.06f);
        beam.closeSubPath();
        juce::ColourGradient bg (k.light.withAlpha (0.35f * std::abs (dir)), lamp.x, lamp.y, k.light.withAlpha (0.0f), lamp.x + dir * f.W() * 0.8f, lamp.y, false);
        g.setGradientFill (bg);
        g.fillPath (beam);
    }

    g.setColour (k.sil.withAlpha (0.95f));
    juce::Path rock;
    rock.addEllipse (cx - f.W() * 0.12f, base - f.H() * 0.03f, f.W() * 0.24f, f.H() * 0.1f);
    g.fillPath (rock);
    juce::Path tower;
    tower.startNewSubPath (cx - h * 0.09f, base);
    tower.lineTo (cx - h * 0.06f, base - h * 0.95f);
    tower.lineTo (cx + h * 0.06f, base - h * 0.95f);
    tower.lineTo (cx + h * 0.09f, base);
    tower.closeSubPath();
    g.fillPath (tower);
    glow (g, lamp, h * 0.15f, k.light.withAlpha (0.9f));
    reflection (g, cx, f.Y (horizon) + 2.0f, f.W() * 0.05f, f.H() * 0.25f, k.light.withAlpha (0.3f), p.t, p.seed);
}
} // namespace

void addLostScenes (std::vector<SceneSpec>& v)
{
    //            name                        category          fn              var  ambient rhythm sparse dark intense  rare
    v.push_back ({ "Sunken city",             Category::Lost,   sunkenCity,     0, { 1.0f, 0.1f, 0.4f, 0.6f, 0.1f }, false });
    v.push_back ({ "Drowned temple",          Category::Lost,   drownedTemple,  0, { 1.0f, 0.1f, 0.5f, 0.5f, 0.1f }, false });
    v.push_back ({ "Columns in the deep",     Category::Lost,   columnsDeep,    0, { 0.9f, 0.2f, 0.4f, 0.7f, 0.2f }, false });
    v.push_back ({ "Mountains in fog",        Category::Lost,   fogMountains,   0, { 1.0f, 0.1f, 0.6f, 0.3f, 0.0f }, false });
    v.push_back ({ "Twin peaks in fog",       Category::Lost,   fogMountains,   1, { 1.0f, 0.1f, 0.5f, 0.4f, 0.1f }, false });
    v.push_back ({ "Moonlit ranges",          Category::Lost,   fogMountains,   2, { 0.9f, 0.1f, 0.7f, 0.3f, 0.0f }, false });
    v.push_back ({ "Ruins in the forest",     Category::Lost,   forestRuins,    0, { 0.7f, 0.2f, 0.6f, 0.6f, 0.1f }, false });
    v.push_back ({ "Lost staircase",          Category::Lost,   staircase,      0, { 0.6f, 0.3f, 0.5f, 0.4f, 0.3f }, false });
    v.push_back ({ "Staircase to nowhere",    Category::Lost,   staircase,      1, { 0.7f, 0.2f, 0.4f, 0.6f, 0.4f }, true  });
    v.push_back ({ "Door in the desert",      Category::Lost,   doorway,        0, { 0.6f, 0.2f, 0.7f, 0.5f, 0.2f }, false });
    v.push_back ({ "Door in the sea",         Category::Lost,   doorway,        1, { 0.7f, 0.1f, 0.6f, 0.6f, 0.2f }, true  });
    v.push_back ({ "Door in the forest",      Category::Lost,   doorway,        2, { 0.5f, 0.2f, 0.6f, 0.7f, 0.3f }, false });
    v.push_back ({ "Pyramid field",           Category::Lost,   pyramids,       0, { 0.6f, 0.4f, 0.3f, 0.5f, 0.5f }, false });
    v.push_back ({ "Star alignment",          Category::Lost,   pyramids,       1, { 0.5f, 0.3f, 0.3f, 0.7f, 0.6f }, false });
    v.push_back ({ "Stone circle",            Category::Lost,   stoneCircle,    0, { 0.7f, 0.3f, 0.4f, 0.6f, 0.3f }, false });
    v.push_back ({ "Runestone in the snow",   Category::Lost,   runestone,      0, { 0.6f, 0.4f, 0.4f, 0.8f, 0.4f }, false });
    v.push_back ({ "Runestone by the fjord",  Category::Lost,   runestone,      1, { 0.7f, 0.3f, 0.4f, 0.7f, 0.3f }, false });
    v.push_back ({ "Torii in the mist",       Category::Lost,   torii,          0, { 0.9f, 0.2f, 0.6f, 0.3f, 0.1f }, false });
    v.push_back ({ "Torii and lanterns",      Category::Lost,   torii,          1, { 0.7f, 0.3f, 0.6f, 0.3f, 0.1f }, false });
    v.push_back ({ "Pagoda on the mountain",  Category::Lost,   pagoda,         0, { 0.8f, 0.2f, 0.5f, 0.3f, 0.1f }, false });
    v.push_back ({ "Pagoda above the clouds", Category::Lost,   pagoda,         1, { 0.9f, 0.1f, 0.5f, 0.3f, 0.1f }, false });
    v.push_back ({ "The buried face",         Category::Lost,   buriedStatue,   0, { 0.5f, 0.2f, 0.4f, 0.8f, 0.3f }, true  });
    v.push_back ({ "Forgotten sea chart",     Category::Lost,   ancientMap,     0, { 0.6f, 0.3f, 0.6f, 0.4f, 0.1f }, false });
    v.push_back ({ "Old star chart",          Category::Lost,   ancientMap,     1, { 0.6f, 0.3f, 0.5f, 0.5f, 0.2f }, false });
    v.push_back ({ "Ziggurat under the moon", Category::Lost,   ziggurat,       0, { 0.6f, 0.4f, 0.4f, 0.6f, 0.4f }, false });
    v.push_back ({ "Floating island",         Category::Lost,   floatingIsland, 0, { 0.8f, 0.2f, 0.4f, 0.3f, 0.3f }, false });
    v.push_back ({ "Obelisk at dusk",         Category::Lost,   obelisk,        0, { 0.6f, 0.3f, 0.6f, 0.4f, 0.2f }, false });
    v.push_back ({ "Twin obelisks",           Category::Lost,   obelisk,        1, { 0.5f, 0.4f, 0.5f, 0.5f, 0.3f }, false });
    v.push_back ({ "Longhouse in the snow",   Category::Lost,   longhouse,      0, { 0.7f, 0.2f, 0.7f, 0.4f, 0.1f }, false });
    v.push_back ({ "Lighthouse on the rock",  Category::Lost,   lighthouse,     0, { 0.7f, 0.3f, 0.7f, 0.4f, 0.1f }, false });
}
}

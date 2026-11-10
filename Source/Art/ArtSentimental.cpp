// =================================================================================================
//  SENTIMENTAL SCENES - tiny pixel-art people in quiet, cinematic places
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
using Pose = kit::Pose;
constexpr float pi = juce::MathConstants<float>::pi;

juce::Colour figureColour (const Frame& f) { return f.ink.light ? f.ink.sil : f.ink.sil.interpolatedWith (juce::Colours::black, 0.6f); }

void ground (G& g, const Frame& f, float v)
{
    g.setColour (f.ink.line.withAlpha (0.35f));
    g.drawLine (f.box.getX(), f.Y (v), f.box.getRight(), f.Y (v), 0.8f);
}

// ------------------------------------------------------------------------------------------------
void benchCouple (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float gy = f.Y (0.92f), px = f.px;
    const float flicker = std::fmod (f.t, 9.0f) < 0.1f ? 0.4f : 1.0f;
    K::lamp (g, f.X (0.3f), gy, f.H() * 0.8f, figureColour (f), k.warm, (0.7f + 0.3f * f.mid) * flicker);
    ground (g, f, 0.92f);

    const float bx = f.X (0.55f);
    const auto col = figureColour (f);
    K::pixelRect (g, bx - px * 8.0f, gy - px * 4.0f, px * 16.0f, px, col);
    K::pixelRect (g, bx - px * 8.0f, gy - px * 7.0f, px * 16.0f, px * 0.8f, col);
    K::pixelRect (g, bx - px * 7.0f, gy - px * 3.0f, px, px * 3.0f, col);
    K::pixelRect (g, bx + px * 6.0f, gy - px * 3.0f, px, px * 3.0f, col);
    K::pixelRect (g, bx - px * 7.0f, gy - px * 7.0f, px, px * 3.0f, col);
    K::pixelRect (g, bx + px * 6.0f, gy - px * 7.0f, px, px * 3.0f, col);

    const float lean = std::sin (f.t * 0.3f) > 0.6f ? -px : 0.0f;
    K::person (g, bx - px * 3.0f, gy - px * 3.0f, px, col, Pose::Sit, 0.0f, 1.0f);
    K::person (g, bx + px * 2.0f + lean, gy - px * 3.0f, px, col, Pose::Sit, 0.0f, 1.0f);
    K::pixelRect (g, bx + px * 1.5f + lean, gy - px * 8.5f, px * 2.0f, px * 0.6f, k.accent.withAlpha (0.8f));   // a scarf
}

void umbrellaCouple (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float gy = f.Y (0.9f), px = f.px;
    K::rain (g, f, f.seed, 70, k.line.withAlpha (0.35f), 0.15f, 0.0f);
    ground (g, f, 0.9f);
    K::reflection (g, f.X (0.5f), gy + 2.0f, f.W() * 0.08f, f.H() * 0.08f, k.warm.withAlpha (0.3f), f.t, f.seed);

    const float x = f.X (0.5f);
    const auto col = figureColour (f);
    const float walk = f.t * 2.0f;
    K::person (g, x - px * 1.5f, gy, px, col, Pose::Walk, walk, 1.0f);
    K::person (g, x + px * 1.5f, gy, px, col, Pose::Walk, walk + 1.0f, 1.0f);

    juce::Path dome;
    dome.startNewSubPath (x - px * 7.0f, gy - px * 11.0f);
    dome.quadraticTo (x, gy - px * 17.0f, x + px * 7.0f, gy - px * 11.0f);
    dome.closeSubPath();
    g.setColour (k.accent.withAlpha (0.7f));
    g.fillPath (dome);
    g.setColour (col);
    g.drawLine (x, gy - px * 14.0f, x, gy - px * 6.0f, juce::jmax (1.0f, px * 0.5f));
    K::halo (g, { x, gy - px * 13.0f }, px * 10.0f, k.warm.withAlpha (0.15f));
}

void loneStreetlight (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float gy = f.Y (0.94f);
    K::lamp (g, f.X (0.4f), gy, f.H() * 0.85f, figureColour (f), k.warm, 0.7f + 0.3f * f.mid);
    ground (g, f, 0.94f);
    K::person (g, f.X (0.6f), gy, f.px * 1.2f, figureColour (f), Pose::Stand, 0.0f, -1.0f);
    K::snow (g, f, f.seed, 25, k.line.withAlpha (0.4f));
}

void plane (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float px = juce::jmax (1.5f, f.px);
    const float x = f.X (-0.05f + 1.1f * f.life);
    const float y = f.Y (0.5f) + std::sin (f.t * 0.6f) * f.H() * 0.1f;
    const float d = f.flip ? -1.0f : 1.0f;
    const auto col = k.line.withAlpha (0.9f);

    for (int i = 1; i < 30; ++i)
    {
        g.setColour (k.line.withAlpha (0.25f * (1.0f - (float) i / 30.0f)));
        g.fillRect (x - d * (px * 6.0f + (float) i * px * 1.5f), y - px * 0.5f, px * 1.2f, px * 0.6f);
    }

    K::pixelRect (g, x - px * 5.0f, y - px, px * 10.0f, px * 2.0f, col);
    K::pixelRect (g, x + d * px * 5.0f - (d < 0.0f ? px : 0.0f), y - px * 0.5f, px, px, col);
    K::pixelRect (g, x - d * px * 5.0f - (d > 0.0f ? px : 0.0f), y - px * 3.0f, px, px * 3.0f, col);
    K::pixelRect (g, x - px * 1.5f, y + px, px * 3.0f, px * 2.0f, col);
    for (int w = -3; w <= 2; w += 2)
        K::pixelRect (g, x + (float) w * px, y - px * 0.5f, px * 0.6f, px * 0.6f, k.warm);

    if (std::fmod (f.t, 1.2f) < 0.1f)
        K::halo (g, { x, y + px * 3.0f }, px * 4.0f, k.line);
    K::halo (g, { x - d * px * 5.0f, y - px * 3.0f }, px * 2.5f, k.accent.withAlpha (0.5f + 0.5f * std::sin (f.t * 5.0f)));
}

void kite (G& g, const Frame& f)
{
    const auto& k = f.ink;
    juce::Path hill;
    hill.startNewSubPath (f.P (0.0f, 1.0f));
    hill.quadraticTo (f.P (0.35f, 0.8f), f.P (0.7f, 1.0f));
    hill.closeSubPath();
    K::silhouette (g, hill, f, 0.2f);

    const Pt hand (f.X (0.32f), f.Y (0.84f) - f.px * 6.0f);
    K::person (g, f.X (0.32f), f.Y (0.84f), f.px, figureColour (f), Pose::Stand, 0.0f, 1.0f);

    const Pt kitePos (f.X (0.72f) + std::sin (f.t * 0.7f) * f.W() * 0.04f, f.Y (0.2f) + std::sin (f.t * 1.1f) * f.H() * 0.05f);
    juce::Path line;
    line.startNewSubPath (hand);
    line.quadraticTo ({ (hand.x + kitePos.x) * 0.5f, hand.y - f.H() * 0.1f }, kitePos);
    g.setColour (k.line.withAlpha (0.4f));
    g.strokePath (line, K::pen (0.6f));

    const float s = f.S() * 0.06f;
    juce::Path diamond;
    diamond.startNewSubPath (kitePos.x, kitePos.y - s);
    diamond.lineTo (kitePos.x + s * 0.7f, kitePos.y);
    diamond.lineTo (kitePos.x, kitePos.y + s * 1.2f);
    diamond.lineTo (kitePos.x - s * 0.7f, kitePos.y);
    diamond.closeSubPath();
    g.setColour (k.accent.withAlpha (0.85f));
    g.fillPath (diamond);

    juce::Path tail;
    tail.startNewSubPath (kitePos.x, kitePos.y + s * 1.2f);
    for (int i = 1; i <= 6; ++i)
        tail.lineTo (kitePos.x + std::sin (f.t * (2.0f + f.mid * 3.0f) + (float) i) * s * 0.6f, kitePos.y + s * 1.2f + (float) i * s * 0.6f);
    g.setColour (k.line.withAlpha (0.5f));
    g.strokePath (tail, K::pen (0.8f));
}

void beachWalk (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float horizon = 0.45f;
    K::moon (g, f.P (0.78f, 0.15f), f.S() * 0.07f, k.line, 1.0f);
    g.setColour (k.line.withAlpha (0.3f));
    g.drawLine (f.box.getX(), f.Y (horizon), f.box.getRight(), f.Y (horizon), 0.7f);
    K::reflection (g, f.X (0.78f), f.Y (horizon) + 2.0f, f.W() * 0.04f, f.H() * 0.3f, k.line.withAlpha (0.4f), f.t, f.seed);
    for (int i = 0; i < 3; ++i)
    {
        const float y = f.Y (0.75f + 0.05f * (float) i) + std::sin (f.t * 0.6f + (float) i) * 2.0f;
        g.setColour (k.line.withAlpha (0.2f));
        g.drawLine (f.box.getX(), y, f.box.getRight(), y + 3.0f, 0.8f);
    }

    const float x = f.X (0.1f + 0.7f * f.life), gy = f.Y (0.92f);
    for (int i = 1; i < 14; ++i)
    {
        g.setColour (k.line.withAlpha (0.3f * (1.0f - (float) i / 14.0f)));
        g.fillEllipse (x - f.DX (0.012f * (float) i) - 1.0f, gy + (float) (i % 2) * 2.0f, 2.0f, 1.5f);
    }
    K::person (g, x, gy, f.px, figureColour (f), Pose::Walk, f.t * 2.0f, 1.0f);
    K::person (g, x + f.DX (0.0f) + f.px * 4.0f * (f.flip ? -1.0f : 1.0f), gy, f.px, figureColour (f), Pose::Walk, f.t * 2.0f + 1.0f, 1.0f);
}

void lakeFigure (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float shore = 0.55f;
    const auto hills = K::ridge (f, f.seed, shore, 0.3f, 0.4f, 50);
    g.setColour (k.sil.interpolatedWith (k.bg, 0.4f));
    g.fillPath (hills);
    {
        G::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::scale (1.0f, -0.5f, 0.0f, f.Y (shore)));
        g.setColour (k.sil.interpolatedWith (k.bg, 0.75f));
        g.fillPath (hills);
    }
    g.setColour (k.line.withAlpha (0.3f));
    g.drawLine (f.box.getX(), f.Y (shore), f.box.getRight(), f.Y (shore), 0.7f);
    K::fog (g, f, shore, 0.08f, k.haze.withAlpha (0.25f));
    K::pixelRect (g, f.box.getX(), f.Y (0.92f), f.W(), f.H() * 0.08f, figureColour (f));
    K::person (g, f.X (0.4f), f.Y (0.92f), f.px * 1.2f, figureColour (f), Pose::Stand, 0.0f, 1.0f);
}

void lighthouse (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float sea = 0.8f;
    const Pt lampPos (f.X (0.3f), f.Y (0.3f));
    const float a = f.t * (0.8f + 0.4f * f.mid);
    for (int side = 0; side < 2; ++side)
    {
        const float dir = std::sin (a + (float) side * pi);
        juce::Path beam;
        beam.startNewSubPath (lampPos);
        beam.lineTo (lampPos.x + dir * f.W() * 0.9f, lampPos.y - f.H() * 0.08f);
        beam.lineTo (lampPos.x + dir * f.W() * 0.9f, lampPos.y + f.H() * 0.08f);
        beam.closeSubPath();
        juce::ColourGradient bg (k.line.withAlpha (0.3f * std::abs (dir)), lampPos.x, lampPos.y, k.line.withAlpha (0.0f), lampPos.x + dir * f.W() * 0.9f, lampPos.y, false);
        g.setGradientFill (bg);
        g.fillPath (beam);
    }
    juce::Path tower;
    tower.startNewSubPath (f.P (0.26f, sea));
    tower.lineTo (f.P (0.28f, 0.33f));
    tower.lineTo (f.P (0.32f, 0.33f));
    tower.lineTo (f.P (0.34f, sea));
    tower.closeSubPath();
    tower.addEllipse (f.X (0.15f), f.Y (sea - 0.03f), f.W() * 0.3f, f.H() * 0.1f);
    K::silhouette (g, tower, f, 0.3f);
    K::halo (g, lampPos, f.S() * 0.08f, k.line);
    g.setColour (k.line.withAlpha (0.3f));
    g.drawLine (f.box.getX(), f.Y (sea), f.box.getRight(), f.Y (sea), 0.7f);
    K::reflection (g, lampPos.x, f.Y (sea) + 3.0f, f.W() * 0.05f, f.H() * 0.18f, k.line.withAlpha (0.3f), f.t, f.seed);
}

void litHouse (G& g, const Frame& f)
{
    const auto& k = f.ink;
    juce::Path hill;
    hill.startNewSubPath (f.P (0.0f, 1.0f));
    hill.lineTo (f.P (0.0f, 0.8f));
    hill.quadraticTo (f.P (0.5f, 0.6f), f.P (1.0f, 0.82f));
    hill.lineTo (f.P (1.0f, 1.0f));
    hill.closeSubPath();
    K::silhouette (g, hill, f, 0.2f);

    const float px = f.px * 1.3f;
    const float hx = f.X (0.55f), hy = f.Y (0.69f);
    const auto col = figureColour (f);
    K::pixelRect (g, hx - px * 5.0f, hy - px * 6.0f, px * 10.0f, px * 6.0f, col);
    juce::Path roof;
    roof.addTriangle (hx - px * 6.5f, hy - px * 6.0f, hx + px * 6.5f, hy - px * 6.0f, hx, hy - px * 11.0f);
    g.setColour (col);
    g.fillPath (roof);
    K::pixelRect (g, hx + px * 2.5f, hy - px * 10.0f, px * 1.5f, px * 3.0f, col);

    const float lampOn = 0.85f + 0.15f * std::sin (f.t * 0.5f);
    K::halo (g, { hx + px * 1.5f, hy - px * 3.5f }, px * 7.0f, k.warm.withAlpha (0.35f * lampOn));
    K::pixelRect (g, hx + px * 0.5f, hy - px * 4.5f, px * 2.0f, px * 2.0f, k.warm.withAlpha (lampOn));
    K::stars (g, f, f.seed, 30, 0.5f, k.line, f.hat);
}

void nightTrain (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float track = f.Y (0.8f);
    g.setColour (k.line.withAlpha (0.35f));
    g.drawLine (f.box.getX(), track, f.box.getRight(), track, 0.8f);

    const float carW = f.H() * 1.1f, carH = f.H() * 0.32f;
    const float total = carW * 6.3f;
    const float lead = f.flip ? f.box.getRight() + total - (f.W() + total * 2.0f) * f.life
                              : f.box.getX() - total + (f.W() + total * 2.0f) * f.life;
    const auto col = figureColour (f);
    for (int c = 0; c < 6; ++c)
    {
        const float x = lead + (f.flip ? 1.0f : -1.0f) * (float) c * carW * 1.05f;
        g.setColour (col);
        g.fillRect (x - carW * 0.5f, track - carH - 2.0f, carW, carH);
        for (int w = 0; w < 5; ++w)
        {
            g.setColour (k.warm.withAlpha (0.85f));
            g.fillRect (x - carW * 0.42f + carW * 0.18f * (float) w, track - carH * 0.8f, carW * 0.1f, carH * 0.35f);
        }
    }
    K::halo (g, { lead + (f.flip ? -1.0f : 1.0f) * carW * 0.5f, track - carH * 0.5f }, carH * 1.5f, k.line.withAlpha (0.35f));
}

void station (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float gy = f.Y (0.88f);
    const auto col = figureColour (f);
    g.setColour (col);
    g.fillRect (f.box.getX(), gy, f.W(), f.H() * 0.06f);
    g.fillRect (f.X (0.08f), f.Y (0.2f), f.W() * 0.84f, f.H() * 0.03f);
    for (int i = 0; i < 3; ++i)
    {
        const float x = f.X (0.2f + 0.3f * (float) i);
        g.fillRect (x, f.Y (0.23f), 2.0f, gy - f.Y (0.23f));
        const float on = (i == 2 && std::fmod (f.t, 6.0f) < 0.15f) ? 0.2f : 1.0f;
        K::halo (g, { x + 1.0f, f.Y (0.26f) }, f.H() * 0.18f, k.warm.withAlpha (0.4f * on));
    }
    const Pt c (f.X (0.62f), f.Y (0.38f));
    const float r = f.H() * 0.06f;
    g.setColour (k.line.withAlpha (0.7f));
    g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.0f);
    g.drawLine (c.x, c.y, c.x + std::sin (f.t * 0.05f) * r * 0.8f, c.y - std::cos (f.t * 0.05f) * r * 0.8f, 0.8f);
    g.drawLine (c.x, c.y, c.x + r * 0.45f, c.y + r * 0.2f, 0.8f);
    K::person (g, f.X (0.42f), gy, f.px * 1.2f, col, Pose::Stand, 0.0f, 1.0f);
}

void stargazer (G& g, const Frame& f)
{
    const auto& k = f.ink;
    {
        G::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::rotation (-0.4f, f.X (0.5f), f.Y (0.35f)));
        K::blob (g, f.P (0.5f, 0.35f), f.W() * 0.6f, f.H() * 0.08f, k.haze.withAlpha (0.3f));
    }
    K::stars (g, f, f.seed, 120, 0.7f, k.line, f.hat);
    juce::Path hill;
    hill.startNewSubPath (f.P (0.0f, 1.0f));
    hill.lineTo (f.P (0.0f, 0.9f));
    hill.quadraticTo (f.P (0.35f, 0.76f), f.P (0.7f, 0.9f));
    hill.lineTo (f.P (1.0f, 0.93f));
    hill.lineTo (f.P (1.0f, 1.0f));
    hill.closeSubPath();
    K::silhouette (g, hill, f, 0.2f);
    K::person (g, f.X (0.35f), f.Y (0.8f), f.px, figureColour (f), Pose::Stand, 0.0f, 1.0f);
}

void rooftopCat (G& g, const Frame& f)
{
    const auto& k = f.ink;
    K::moon (g, f.P (0.7f, 0.25f), f.S() * 0.16f, k.line.withAlpha (k.light ? 0.25f : 0.8f), 1.0f);
    juce::Path roofs;
    for (int i = 0; i < 5; ++i)
    {
        const float u = (float) i / 5.0f, w = 0.2f;
        const float top = 0.6f + 0.15f * K::hash (f.seed, i);
        roofs.startNewSubPath (f.P (u, 1.0f));
        roofs.lineTo (f.P (u, top + 0.08f));
        roofs.lineTo (f.P (u + w * 0.5f, top));
        roofs.lineTo (f.P (u + w, top + 0.08f));
        roofs.lineTo (f.P (u + w, 1.0f));
        roofs.closeSubPath();
    }
    K::silhouette (g, roofs, f, 0.2f);

    const float px = f.px * 1.2f;
    const float top = 0.6f + 0.15f * K::hash (f.seed, 2);
    const float cx = f.X (0.5f), cy = f.Y (top);
    const auto col = figureColour (f);
    K::pixelRect (g, cx - px * 1.5f, cy - px * 4.0f, px * 3.0f, px * 4.0f, col);
    K::pixelRect (g, cx - px * 1.0f, cy - px * 6.0f, px * 2.0f, px * 2.0f, col);
    K::pixelRect (g, cx - px * 1.0f, cy - px * 7.0f, px * 0.6f, px, col);
    K::pixelRect (g, cx + px * 0.4f, cy - px * 7.0f, px * 0.6f, px, col);
    const float tailUp = std::sin (f.t * 1.5f) > 0.0f ? 1.0f : 0.0f;
    K::pixelRect (g, cx + px * 1.5f, cy - px * (1.0f + tailUp * 2.0f), px, px * (1.0f + tailUp * 2.0f), col);
}

void rainWindow (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const Rc win (f.X (0.15f) - (f.flip ? f.W() * 0.7f : 0.0f), f.Y (0.08f), f.W() * 0.7f, f.H() * 0.84f);
    for (int i = 0; i < 14; ++i)
    {
        const Pt c (win.getX() + win.getWidth() * K::hash (f.seed, i), win.getY() + win.getHeight() * (0.4f + 0.6f * K::hash (f.seed, i + 40)));
        K::halo (g, c, win.getWidth() * (0.04f + 0.05f * K::hash (f.seed, i + 80)), (K::hash (f.seed, i + 9) > 0.5f ? k.warm : k.glow).withAlpha (0.35f));
    }
    for (int i = 0; i < 50; ++i)
    {
        const float y = std::fmod (K::hash (f.seed, i + 300) + f.t * (0.03f + 0.08f * K::hash (f.seed, i + 200)), 1.0f);
        const float x = win.getX() + win.getWidth() * K::hash (f.seed, i + 400);
        const float yy = win.getY() + win.getHeight() * y;
        g.setColour (k.line.withAlpha (0.35f));
        g.drawLine (x, yy, x, yy - win.getHeight() * 0.04f, 0.7f);
        g.fillEllipse (x - 1.0f, yy - 1.0f, 2.0f, 2.0f);
    }
    g.setColour (figureColour (f));
    g.drawRect (win, juce::jmax (2.0f, f.S() * 0.02f));
    g.fillRect (win.getCentreX() - 1.5f, win.getY(), 3.0f, win.getHeight());
    g.fillRect (win.getX(), win.getCentreY() - 1.5f, win.getWidth(), 3.0f);
    K::person (g, win.getRight() - win.getWidth() * 0.15f, win.getBottom(), f.px * 2.0f, figureColour (f), Pose::Stand, 0.0f, -1.0f);
}

void lanternBoat (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float water = f.Y (0.7f);
    g.setColour (k.line.withAlpha (0.3f));
    g.drawLine (f.box.getX(), water, f.box.getRight(), water, 0.7f);
    const float x = f.X (0.05f + 0.9f * f.life);
    const float bob = std::sin (f.t * 1.2f) * f.px;
    const float px = f.px;
    const auto col = figureColour (f);
    juce::Path hull;
    hull.startNewSubPath (x - px * 9.0f, water - px * 2.0f + bob);
    hull.lineTo (x + px * 9.0f, water - px * 2.0f + bob);
    hull.lineTo (x + px * 6.0f, water + px + bob);
    hull.lineTo (x - px * 6.0f, water + px + bob);
    hull.closeSubPath();
    g.setColour (col);
    g.fillPath (hull);
    K::person (g, x - px * 2.0f, water - px * 2.0f + bob, px, col, Pose::Sit, 0.0f, 1.0f);
    g.drawLine (x + px * 5.0f, water - px * 2.0f + bob, x + px * 5.0f, water - px * 9.0f + bob, 1.0f);
    const Pt lantern (x + px * 5.0f, water - px * 9.0f + bob);
    K::halo (g, lantern, px * 8.0f, k.warm.withAlpha (0.5f + 0.2f * f.mid));
    K::pixelRect (g, lantern.x - px, lantern.y, px * 2.0f, px * 2.0f, k.warm);
    K::reflection (g, lantern.x, water + 3.0f, px * 6.0f, f.H() * 0.25f, k.warm.withAlpha (0.4f), f.t, f.seed);
}

void desertWalker (G& g, const Frame& f)
{
    const auto& k = f.ink;
    K::stars (g, f, f.seed, 50, 0.5f, k.line, f.hat);
    g.setColour (k.sil.interpolatedWith (k.bg, 0.5f));
    g.fillPath (K::ridge (f, f.seed, 0.75f, 0.15f, 0.0f, 40));
    const auto dunes = K::ridge (f, f.seed + 5u, 0.88f, 0.1f, 0.0f, 40);
    K::silhouette (g, dunes, f, 0.15f);
    const float x = f.X (0.2f + 0.5f * f.life);
    K::person (g, x, f.Y (0.84f), juce::jmax (1.0f, f.px * 0.7f), figureColour (f), Pose::Walk, f.t * 1.6f, 1.0f);
}

void swing (G& g, const Frame& f)
{
    const auto& k = f.ink;
    ground (g, f, 0.95f);
    juce::Path trunk;
    trunk.startNewSubPath (f.P (0.25f, 0.95f));
    trunk.quadraticTo (f.P (0.22f, 0.5f), f.P (0.3f, 0.2f));
    trunk.quadraticTo (f.P (0.55f, 0.12f), f.P (0.85f, 0.18f));
    g.setColour (figureColour (f));
    g.strokePath (trunk, K::pen (juce::jmax (2.0f, f.S() * 0.03f)));
    K::blob (g, f.P (0.45f, 0.15f), f.W() * 0.35f, f.H() * 0.15f, k.sil.withAlpha (k.light ? 0.6f : 0.9f));

    const Pt pivot (f.X (0.65f), f.Y (0.17f));
    const float swingAngle = std::sin (f.t * (1.2f + 0.6f * f.mid)) * (0.35f + 0.2f * f.energy);
    const float len = f.H() * 0.58f;
    const Pt seat (pivot.x + std::sin (swingAngle) * len, pivot.y + std::cos (swingAngle) * len);
    g.setColour (k.line.withAlpha (0.5f));
    g.drawLine (pivot.x - 4.0f, pivot.y, seat.x - 4.0f, seat.y, 0.7f);
    g.drawLine (pivot.x + 4.0f, pivot.y, seat.x + 4.0f, seat.y, 0.7f);
    K::pixelRect (g, seat.x - f.px * 3.0f, seat.y, f.px * 6.0f, f.px, figureColour (f));
    K::person (g, seat.x, seat.y, f.px, figureColour (f), Pose::Sit, 0.0f, 1.0f);
}

void telescope (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto star = f.P (0.8f, 0.15f);
    K::halo (g, star, f.S() * 0.12f, k.line.withAlpha (0.7f + 0.3f * f.hat));
    juce::Path hill;
    hill.startNewSubPath (f.P (0.0f, 1.0f));
    hill.quadraticTo (f.P (0.3f, 0.75f), f.P (0.65f, 1.0f));
    hill.closeSubPath();
    K::silhouette (g, hill, f, 0.2f);

    const float gy = f.Y (0.82f), px = f.px;
    const auto col = figureColour (f);
    K::person (g, f.X (0.28f), gy, px, col, Pose::Stand, 0.0f, 1.0f);
    const Pt eye (f.X (0.28f) + px * 2.0f * (f.flip ? -1.0f : 1.0f), gy - px * 8.0f);
    g.setColour (col);
    g.drawLine (eye.x, eye.y, eye.x + (star.x - eye.x) * 0.12f, eye.y + (star.y - eye.y) * 0.12f, px * 1.5f);
    g.drawLine (eye.x + f.DX (0.02f), eye.y + px * 2.0f, eye.x + f.DX (0.02f), gy, 1.0f);
    g.setColour (k.line.withAlpha (0.12f));
    g.drawLine (eye.x, eye.y, star.x, star.y, 0.6f);
}

void synthCameo (G& g, const Frame& f)
{
    // the secret: a tiny musician at a synthesiser (placeholder for a portrait-based sprite)
    const auto& k = f.ink;
    const float px = f.px * 1.3f;
    const float gy = f.Y (0.9f), cx = f.X (0.5f);
    const auto col = figureColour (f);
    K::halo (g, { cx, gy - px * 6.0f }, px * 16.0f, k.glow.withAlpha (0.3f));
    K::person (g, cx, gy - px * 4.0f, px, col, Pose::Stand, 0.0f, 1.0f);
    K::pixelRect (g, cx - px * 2.0f, gy - px * 13.5f, px * 4.0f, px * 0.8f, k.accent);       // headphones band
    K::pixelRect (g, cx - px * 2.0f, gy - px * 13.0f, px * 0.8f, px * 1.6f, k.accent);
    K::pixelRect (g, cx + px * 1.2f, gy - px * 13.0f, px * 0.8f, px * 1.6f, k.accent);

    K::pixelRect (g, cx - px * 10.0f, gy - px * 4.0f, px * 20.0f, px * 4.0f, col);
    for (int i = 0; i < 12; ++i)
    {
        const bool lit = (i < 4 && f.kick > 0.3f) || (i >= 8 && f.hat > 0.3f) || (i >= 4 && i < 8 && f.mid > 0.6f && K::hash (f.seed + (juce::uint32) (f.t * 8.0f), i) > 0.6f);
        K::pixelRect (g, cx - px * 9.0f + (float) i * px * 1.5f, gy - px * 3.0f, px, px * 2.0f, lit ? k.accent : k.line.withAlpha (0.6f));
    }
    ground (g, f, 0.9f);
}
} // namespace

void addSentimental (std::vector<Spec>& v)
{
    const int hero = flagHero, comp = flagCompanion;
    //            name                          category                fn              zone            size  aspect  life           quiet bass  melod energ   flags
    v.push_back ({ "Two on a bench",             Category::Sentimental, benchCouple,    Zone::Ground,   0.5f,  1.5f, 18.0f, 30.0f, { 1.0f, 0.1f, 0.8f, 0.0f }, hero });
    v.push_back ({ "Under one umbrella",         Category::Sentimental, umbrellaCouple, Zone::Ground,   0.48f, 1.2f, 14.0f, 24.0f, { 0.9f, 0.2f, 0.8f, 0.1f }, hero });
    v.push_back ({ "Beneath the streetlight",    Category::Sentimental, loneStreetlight,Zone::Ground,   0.55f, 0.9f, 14.0f, 24.0f, { 1.0f, 0.2f, 0.6f, 0.0f }, hero });
    v.push_back ({ "Little aeroplane",           Category::Sentimental, plane,          Zone::BandSky,  0.18f, 1.0f, 14.0f, 20.0f, { 0.8f, 0.1f, 0.8f, 0.2f }, comp });
    v.push_back ({ "Kite on the hill",           Category::Sentimental, kite,           Zone::Ground,   0.65f, 1.2f, 14.0f, 24.0f, { 0.7f, 0.1f, 1.0f, 0.2f }, hero });
    v.push_back ({ "Two on the night beach",     Category::Sentimental, beachWalk,      Zone::Ground,   0.5f,  2.4f, 18.0f, 30.0f, { 0.9f, 0.1f, 0.9f, 0.0f }, hero });
    v.push_back ({ "Figure by the lake",         Category::Sentimental, lakeFigure,     Zone::Ground,   0.5f,  2.2f, 16.0f, 28.0f, { 1.0f, 0.2f, 0.7f, 0.0f }, hero });
    v.push_back ({ "Lighthouse",                 Category::Sentimental, lighthouse,     Zone::Ground,   0.65f, 1.4f, 14.0f, 26.0f, { 0.8f, 0.3f, 0.6f, 0.2f }, hero });
    v.push_back ({ "One lit window",             Category::Sentimental, litHouse,       Zone::Ground,   0.45f, 1.8f, 16.0f, 30.0f, { 1.0f, 0.1f, 0.7f, 0.0f }, hero });
    v.push_back ({ "Night train",                Category::Sentimental, nightTrain,     Zone::BandLow,  0.22f, 1.0f, 12.0f, 18.0f, { 0.6f, 0.4f, 0.6f, 0.5f }, comp });
    v.push_back ({ "Waiting at the station",     Category::Sentimental, station,        Zone::Ground,   0.5f,  1.8f, 14.0f, 26.0f, { 0.9f, 0.2f, 0.6f, 0.1f }, hero });
    v.push_back ({ "Watching the stars",         Category::Sentimental, stargazer,      Zone::Ground,   0.6f,  1.6f, 16.0f, 30.0f, { 1.0f, 0.1f, 0.8f, 0.0f }, hero });
    v.push_back ({ "Cat on the rooftops",        Category::Sentimental, rooftopCat,     Zone::Ground,   0.45f, 2.0f, 12.0f, 22.0f, { 0.8f, 0.2f, 0.6f, 0.2f }, hero });
    v.push_back ({ "Window in the rain",         Category::Sentimental, rainWindow,     Zone::Centre,   0.62f, 1.1f, 14.0f, 26.0f, { 0.9f, 0.2f, 0.8f, 0.1f }, hero });
    v.push_back ({ "Lantern boat",               Category::Sentimental, lanternBoat,    Zone::BandLow,  0.3f,  1.0f, 18.0f, 28.0f, { 1.0f, 0.1f, 0.8f, 0.0f }, hero | comp });
    v.push_back ({ "Walker in the desert",       Category::Sentimental, desertWalker,   Zone::Ground,   0.5f,  3.0f, 18.0f, 30.0f, { 0.9f, 0.2f, 0.7f, 0.1f }, hero });
    v.push_back ({ "The swing",                  Category::Sentimental, swing,          Zone::Ground,   0.7f,  1.1f, 14.0f, 24.0f, { 0.6f, 0.2f, 0.9f, 0.4f }, hero });
    v.push_back ({ "The telescope",              Category::Sentimental, telescope,      Zone::Ground,   0.5f,  1.5f, 14.0f, 24.0f, { 0.9f, 0.1f, 0.7f, 0.1f }, hero });
    v.push_back ({ "Musician at the synth",      Category::Sentimental, synthCameo,     Zone::Ground,   0.4f,  1.4f, 14.0f, 20.0f, { 0.4f, 0.6f, 0.6f, 0.7f }, hero | flagRare });
}
}

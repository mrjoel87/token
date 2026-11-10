// =================================================================================================
//  GHOSTS, OCCULT & HIDDEN MESSAGES
//  Historical symbols are drawn as documented (alchemical elements and planetary metals, Maier's
//  squared circle, Elder Futhark, Ogham, real constellations). Anything invented is named as such.
//  No fake "ancient text": manuscript writing is deliberately abstract.
// =================================================================================================
#include "SceneKit.h"
#include "../World/PixelArt.h"
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
constexpr float twoPi = juce::MathConstants<float>::twoPi;

/** Electronic static, re-seeded ~20 times a second, denser inside an ellipse. */
void staticField (G& g, const Frame& f, const P& p, int n, juce::Point<float> focus, float focusR, float focusBoost)
{
    const int frame = (int) (p.t * 20.0f);
    for (int i = 0; i < n; ++i)
    {
        float x = hash (p.seed + (juce::uint32) frame, i * 2);
        float y = hash (p.seed + (juce::uint32) frame, i * 2 + 1);

        if (focusBoost > 0.0f && i % 2 == 0)
        {
            const float a = x * twoPi, r = std::sqrt (y) * focusR;
            x = (focus.x + std::sin (a) * r - f.r.getX()) / f.W();
            y = (focus.y + std::cos (a) * r * 1.3f - f.r.getY()) / f.H();
        }

        const float b = hash (p.seed + 77u, i + frame);
        g.setColour (p.ink.light.withAlpha (0.08f + 0.35f * b));
        g.fillRect (f.X (x), f.Y (y), 1.5f, 1.5f);
    }
}

void ghostFace (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const int faces = p.variant == 2 ? 2 : 1;

    for (int i = 0; i < faces; ++i)
    {
        const float cx = f.X (faces == 1 ? 0.5f : 0.33f + 0.34f * (float) i);
        const float cy = f.Y (0.48f);
        const float s = f.H() * (faces == 1 ? 0.32f : 0.24f);
        const float stretch = p.variant == 1 ? 1.35f : 1.0f;
        const float breathe = 1.0f + 0.04f * std::sin (p.t * 0.9f + (float) i);

        staticField (g, f, p, 260, { cx, cy }, s * 0.9f, 1.0f);

        G::ScopedSaveState state (g);
        if (p.variant == 1)
            g.addTransform (juce::AffineTransform::rotation (0.15f * std::sin (p.t * 0.4f), cx, cy));

        softEllipse (g, { cx, cy }, s * 0.62f * breathe, s * 0.85f * breathe * stretch, k.light.withAlpha (0.35f + 0.25f * p.mid));
        const float eyeY = cy - s * 0.12f * stretch;
        const float open = 0.6f + 0.4f * std::sin (p.t * 0.6f + (float) i);
        softEllipse (g, { cx - s * 0.22f, eyeY }, s * 0.14f, s * 0.11f * open, k.deep.withAlpha (0.95f));
        softEllipse (g, { cx + s * 0.22f, eyeY }, s * 0.14f, s * 0.11f * open, k.deep.withAlpha (0.95f));
        softEllipse (g, { cx, cy + s * 0.38f * stretch }, s * 0.1f, s * (0.04f + 0.1f * p.mid), k.deep.withAlpha (0.9f));

        if (p.mystery > 0.5f)
        {
            glow (g, { cx - s * 0.22f, eyeY }, s * 0.05f, k.accent.withAlpha ((p.mystery - 0.5f) * 1.2f));
            glow (g, { cx + s * 0.22f, eyeY }, s * 0.05f, k.accent.withAlpha ((p.mystery - 0.5f) * 1.2f));
        }
    }
}

void silhouetteStatic (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const float cx = f.X (0.5f);

    if (p.variant == 2)
    {
        // corridor in one-point perspective
        const juce::Point<float> vp (cx, f.Y (0.45f));
        g.setColour (k.light.withAlpha (0.25f));
        for (int i = 0; i < 4; ++i)
        {
            const float cxr = (float) (i % 2) == 0.0f ? 0.0f : 1.0f;
            const float cyr = i < 2 ? 0.0f : 1.0f;
            g.drawLine (f.X (cxr), f.Y (cyr), vp.x + (cxr - 0.5f) * f.W() * 0.12f, vp.y + (cyr - 0.5f) * f.H() * 0.2f, 1.0f);
        }
        for (int i = 1; i < 6; ++i)
        {
            const float s = 1.0f / (1.0f + (float) i * 0.8f);
            g.drawRect (vp.x - f.W() * 0.5f * s, vp.y - f.H() * 0.5f * s, f.W() * s, f.H() * s, 0.6f);
        }
        glow (g, vp, f.H() * 0.15f, k.light.withAlpha (0.5f));
        figure (g, vp.x, vp.y + f.H() * 0.1f, f.H() * 0.2f, k.sil, 0.0f, false);
        staticField (g, f, p, 300, vp, 0.0f, 0.0f);
        return;
    }

    staticField (g, f, p, 500, { cx, f.Y (0.5f) }, f.H() * 0.4f, 1.0f);

    if (p.variant == 1)
    {
        const float dw = f.W() * 0.14f, dh = f.H() * 0.62f;
        glow (g, { cx, f.Y (0.55f) }, dh * 0.7f, k.light.withAlpha (0.3f));
        g.setColour (k.light.withAlpha (0.55f));
        g.fillRect (cx - dw * 0.5f, f.Y (0.88f) - dh, dw, dh);
    }

    const float flicker = 0.75f + 0.25f * std::sin (p.t * 13.0f) * hash (p.seed, (int) (p.t * 7.0f));
    figure (g, cx, f.Y (0.88f), f.H() * 0.5f, k.sil.withAlpha (flicker), 0.0f, false);
}

void alchemy (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const float glowAmt = 0.45f + 0.5f * p.bass;
    const float th = juce::jmax (1.0f, f.H() * 0.006f);

    if (p.variant == 0)
    {
        // the four classical elements: fire, water, air, earth
        const char* specs[] = { "50,10 90,85 10,85 50,10",
                                "10,15 90,15 50,90 10,15",
                                "50,10 90,85 10,85 50,10|22,58 78,58",
                                "10,15 90,15 50,90 10,15|22,42 78,42" };
        for (int i = 0; i < 4; ++i)
        {
            const float a = (float) i / 4.0f * twoPi + p.t * 0.05f;
            const auto box = R (f.H() * 0.18f, f.H() * 0.18f).withCentre ({ f.X (0.5f) + std::sin (a) * f.H() * 0.26f, f.Y (0.5f) - std::cos (a) * f.H() * 0.26f });
            glyph (g, specs[i], box, k.accent, th, glowAmt);
        }
        juce::Path ring;
        ring.addEllipse (f.X (0.5f) - f.H() * 0.4f, f.Y (0.5f) - f.H() * 0.4f, f.H() * 0.8f, f.H() * 0.8f);
        glowStroke (g, ring, k.light, th * 0.7f, 0.4f);
    }
    else if (p.variant == 1)
    {
        // squared circle (after Michael Maier, Atalanta Fugiens, 1617)
        const auto c = juce::Point<float> (f.X (0.5f), f.Y (0.52f));
        const float R0 = f.H() * 0.4f;
        juce::Path pth;
        pth.addEllipse (c.x - R0, c.y - R0, R0 * 2.0f, R0 * 2.0f);
        const float tri = R0 * 0.98f;
        pth.addTriangle (c.x, c.y - tri, c.x + tri * 0.866f, c.y + tri * 0.5f, c.x - tri * 0.866f, c.y + tri * 0.5f);
        const float sq = tri * 0.5f * 0.9f;
        pth.addRectangle (c.x - sq, c.y - sq + tri * 0.05f, sq * 2.0f, sq * 2.0f);
        pth.addEllipse (c.x - sq * 0.7f, c.y - sq * 0.7f + tri * 0.05f, sq * 1.4f, sq * 1.4f);
        glowStroke (g, pth, k.accent, th, glowAmt);
        figure (g, c.x + R0 * 1.25f, c.y + R0, R0 * 0.5f, k.sil, 0.0f, false);
    }
    else
    {
        // the seven planetary metals around a sun
        const char* specs[] = {
            "70,15 50,10 35,25 30,50 35,75 50,90 70,85 55,70 50,50 55,30 70,15",   // moon (silver)
            "32,12 40,22 60,22 68,12|50,63 50,95|36,80 64,80",                      // mercury (horns + cross)
            "50,57 50,95|32,78 68,78",                                              // venus (cross)
            "56,44 85,15|62,15 85,15 85,38",                                        // mars (arrow)
            "25,30 40,18 50,30 28,70 85,70|65,40 65,95",                            // jupiter (tin)
            "35,5 35,60|20,20 50,20|35,45 50,35 66,45 58,72 70,92",                 // saturn (lead)
        };
        const juce::Point<float> circles[] = { { 0, 0 }, { 50, 45 }, { 50, 35 }, { 40, 60 }, { -1, -1 }, { -1, -1 } };
        const float radii[] = { 0, 17, 21, 22, 0, 0 };

        const auto c = juce::Point<float> (f.X (0.5f), f.Y (0.5f));
        juce::Path sun;
        sun.addEllipse (c.x - f.H() * 0.07f, c.y - f.H() * 0.07f, f.H() * 0.14f, f.H() * 0.14f);
        sun.addEllipse (c.x - 2.0f, c.y - 2.0f, 4.0f, 4.0f);
        glowStroke (g, sun, k.warm, th, glowAmt);

        for (int i = 0; i < 6; ++i)
        {
            const float a = (float) i / 6.0f * twoPi - p.t * 0.04f;
            const auto box = R (f.H() * 0.15f, f.H() * 0.15f).withCentre ({ c.x + std::sin (a) * f.H() * 0.32f, c.y - std::cos (a) * f.H() * 0.32f });
            glyph (g, specs[i], box, k.accent, th, glowAmt);

            if (radii[i] > 0.0f)
            {
                juce::Path circle;
                const float cr = radii[i] / 100.0f * box.getWidth();
                circle.addEllipse (box.getX() + circles[i].x / 100.0f * box.getWidth() - cr,
                                   box.getY() + circles[i].y / 100.0f * box.getHeight() - cr, cr * 2.0f, cr * 2.0f);
                glowStroke (g, circle, k.accent, th, glowAmt);
            }
        }
    }
}

void inventedSigil (G& g, R area, const P& p)
{
    // Invented, fictional sigils - generated from the seed, not taken from any tradition.
    const Frame f { area };
    const auto& k = p.ink;
    const auto c = juce::Point<float> (f.X (0.5f), f.Y (0.5f));
    const float Rr = f.H() * 0.36f;
    const juce::uint32 s = p.seed + (juce::uint32) p.variant * 1013u;
    const int n = 5 + (int) (hash (s, 0) * 4.0f);

    juce::Path sig;
    sig.addEllipse (c.x - Rr, c.y - Rr, Rr * 2.0f, Rr * 2.0f);
    juce::Point<float> prev;
    for (int i = 0; i <= n; ++i)
    {
        const int idx = (int) (hash (s, i + 1) * 12.0f);
        const float a = (float) idx / 12.0f * twoPi;
        const juce::Point<float> v (c.x + std::sin (a) * Rr * 0.8f, c.y - std::cos (a) * Rr * 0.8f);
        if (i == 0) sig.startNewSubPath (v); else sig.lineTo (v);
        if (i == 0 || i == n)
            sig.addEllipse (v.x - Rr * 0.05f, v.y - Rr * 0.05f, Rr * 0.1f, Rr * 0.1f);
        if (i > 0) sig.startNewSubPath (v);
        prev = v;
    }
    juce::ignoreUnused (prev);

    const float reveal = juce::jlimit (0.0f, 1.0f, p.t / 3.0f);
    glowStroke (g, sig, k.accent, juce::jmax (1.0f, f.H() * 0.006f), (0.3f + 0.6f * p.bass) * reveal);
}

void alphabet (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const float th = juce::jmax (1.0f, f.H() * 0.006f);
    const float glowAmt = 0.4f + 0.5f * p.mid;

    if (p.variant == 0)
    {
        // a line of Elder Futhark runes, appearing one by one
        const int n = 9;
        const int shown = juce::jmin (n, (int) (p.t * 1.5f) + 1);
        for (int i = 0; i < shown; ++i)
        {
            const auto box = R (f.W() * 0.05f, f.H() * 0.22f).withCentre ({ f.X (0.1f + 0.8f * (float) i / (float) (n - 1)), f.Y (0.5f) });
            pixel::drawRune (g, (int) (hash (p.seed, i) * 24.0f), box, k.accent, glowAmt, th);
        }
    }
    else if (p.variant == 1)
    {
        // Ogham: strokes along a stem line (letters from the four aicmí)
        const float y = f.Y (0.5f), len = f.H() * 0.12f;
        juce::Path stem;
        stem.startNewSubPath (f.X (0.08f), y);
        stem.lineTo (f.X (0.92f), y);
        float x = f.X (0.12f);
        const int letters = 12;
        for (int i = 0; i < letters && x < f.X (0.88f); ++i)
        {
            const int aicme = (int) (hash (p.seed, i) * 4.0f);
            const int count = 1 + (int) (hash (p.seed, i + 50) * 5.0f);
            for (int s = 0; s < count; ++s)
            {
                const float sx = x + (float) s * len * 0.25f;
                if (aicme == 0)      { stem.startNewSubPath (sx, y); stem.lineTo (sx, y - len); }
                else if (aicme == 1) { stem.startNewSubPath (sx, y); stem.lineTo (sx, y + len); }
                else if (aicme == 2) { stem.startNewSubPath (sx - len * 0.3f, y + len * 0.7f); stem.lineTo (sx + len * 0.3f, y - len * 0.7f); }
                else                 { stem.startNewSubPath (sx, y - len * 0.3f); stem.lineTo (sx, y + len * 0.3f); }
            }
            x += (float) count * len * 0.25f + len * 0.6f;
        }
        glowStroke (g, stem, k.accent, th, glowAmt);
    }
    else
    {
        // the full futhark in a ring
        const auto c = juce::Point<float> (f.X (0.5f), f.Y (0.5f));
        for (int i = 0; i < 24; ++i)
        {
            const float a = (float) i / 24.0f * twoPi + p.t * 0.03f;
            const auto box = R (f.H() * 0.05f, f.H() * 0.08f).withCentre ({ c.x + std::sin (a) * f.H() * 0.36f, c.y - std::cos (a) * f.H() * 0.36f });
            pixel::drawRune (g, i, box, k.accent, glowAmt * (0.5f + 0.5f * std::sin (p.t * 2.0f + (float) i)), th * 0.8f);
        }
    }
}

void sacredGeometry (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const auto c = juce::Point<float> (f.X (0.5f), f.Y (0.5f));
    const float r = f.H() * (p.variant == 2 ? 0.1f : 0.14f);
    const float rot = p.t * 0.05f;
    juce::Path pth;

    std::vector<juce::Point<float>> centres;
    if (p.variant == 1)
    {
        centres = { { c.x - r * 0.5f, c.y }, { c.x + r * 0.5f, c.y } };   // vesica piscis
    }
    else
    {
        centres.push_back (c);
        for (int ring = 1; ring <= (p.variant == 2 ? 2 : 1); ++ring)
            for (int i = 0; i < 6; ++i)
            {
                const float a = rot + (float) i / 6.0f * twoPi;
                centres.push_back ({ c.x + std::sin (a) * r * (float) ring, c.y - std::cos (a) * r * (float) ring });
            }
    }

    for (const auto& pt : centres)
        pth.addEllipse (pt.x - r, pt.y - r, r * 2.0f, r * 2.0f);

    if (p.variant == 2)   // Metatron's cube: connect every centre
        for (size_t i = 0; i < centres.size(); ++i)
            for (size_t j = i + 1; j < centres.size(); ++j)
            {
                pth.startNewSubPath (centres[i]);
                pth.lineTo (centres[j]);
            }

    glowStroke (g, pth, k.accent, juce::jmax (0.8f, f.H() * 0.004f), 0.35f + 0.5f * p.bass);
}

void eyes (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const int n = p.variant == 1 ? 7 : 1;

    for (int i = 0; i < n; ++i)
    {
        const float cx = n == 1 ? f.X (0.5f) : f.X (0.12f + 0.76f * hash (p.seed, i));
        const float cy = n == 1 ? f.Y (0.5f) : f.Y (0.2f + 0.6f * hash (p.seed, i + 30));
        const float w = n == 1 ? f.W() * 0.36f : f.W() * 0.08f;
        const float delay = n == 1 ? 0.0f : hash (p.seed, i + 60) * 3.0f;
        const float open = juce::jlimit (0.0f, 1.0f, (p.t - delay) * 0.8f) * (std::fmod (p.t + (float) i, 4.0f) < 0.15f ? 0.1f : 1.0f);
        const float h = w * 0.38f * open;
        if (h < 0.5f)
            continue;

        juce::Path almond;
        almond.startNewSubPath (cx - w * 0.5f, cy);
        almond.quadraticTo (cx, cy - h, cx + w * 0.5f, cy);
        almond.quadraticTo (cx, cy + h, cx - w * 0.5f, cy);
        almond.closeSubPath();

        g.setColour (k.deep.withAlpha (0.8f));
        g.fillPath (almond);
        {
            G::ScopedSaveState state (g);
            g.reduceClipRegion (almond);
            const float look = std::sin (p.t * 0.7f + (float) i) * w * 0.12f;
            const float ir = w * 0.17f;
            juce::ColourGradient iris (k.accent, cx + look, cy, k.deep, cx + look + ir, cy, true);
            g.setGradientFill (iris);
            g.fillEllipse (cx + look - ir, cy - ir, ir * 2.0f, ir * 2.0f);
            g.setColour (k.deep);
            const float pr = ir * (0.4f - 0.15f * p.kick);
            g.fillEllipse (cx + look - pr, cy - pr, pr * 2.0f, pr * 2.0f);
            g.setColour (k.light.withAlpha (0.8f));
            g.fillEllipse (cx + look - ir * 0.45f, cy - ir * 0.5f, ir * 0.2f, ir * 0.2f);
        }
        glowStroke (g, almond, k.light, juce::jmax (0.8f, w * 0.01f), 0.6f);
    }
}

void numbers (G& g, R area, const P& p)
{
    // A fictional "number station" transmission - invented digits, no hidden meaning.
    const Frame f { area };
    const auto& k = p.ink;
    const float th = juce::jmax (1.0f, f.H() * 0.008f);

    if (p.variant == 0)
    {
        const int rows = 4, groups = 3, perGroup = 5;
        const int total = rows * groups * perGroup;
        const int shown = juce::jmin (total, (int) (p.t * 9.0f));
        const float cw = f.W() * 0.032f, chh = f.H() * 0.09f;

        for (int d = 0; d < shown; ++d)
        {
            const int row = d / (groups * perGroup), col = d % (groups * perGroup);
            const int grp = col / perGroup, pos = col % perGroup;
            const float x = f.X (0.1f) + (float) grp * cw * 7.5f + (float) pos * cw * 1.4f;
            const float y = f.Y (0.2f) + (float) row * chh * 1.7f;
            const bool fresh = d >= shown - 3;
            sevenSegment (g, (int) (hash (p.seed, d) * 10.0f), R (x, y, cw, chh),
                          (fresh ? k.light : k.accent).withAlpha (fresh ? 0.95f : 0.6f), th);
        }
    }
    else
    {
        const float cw = f.W() * 0.05f, chh = f.H() * 0.16f;
        for (int line = 0; line < 2; ++line)
            for (int d = 0; d < 7; ++d)
            {
                const float flick = hash (p.seed + (juce::uint32) (p.t * 6.0f), d + line * 10);
                const int digit = flick > 0.9f ? (int) (flick * 97.0f) % 10 : (int) (hash (p.seed, d + line * 10) * 10.0f);
                const float x = f.X (0.18f) + (float) d * cw * 1.5f + (d > 1 ? cw * 0.8f : 0.0f);
                sevenSegment (g, digit, R (x, f.Y (0.25f + 0.32f * (float) line), cw, chh), k.accent.withAlpha (0.8f), th * 1.3f);
            }
        g.setColour (k.accent.withAlpha (0.8f));
        g.drawEllipse (f.X (0.18f) + cw * 3.2f, f.Y (0.23f), cw * 0.35f, cw * 0.35f, th);
    }
}

void constellation (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    stars (g, f, p.seed, 70, 1.0f, k.light.withAlpha (0.35f), p.t);

    std::vector<juce::Point<float>> s;
    std::vector<std::pair<int, int>> lines;

    if (p.variant == 0)        // Orion
    {
        s = { { 0.30f, 0.20f }, { 0.62f, 0.25f }, { 0.48f, 0.12f }, { 0.42f, 0.50f }, { 0.48f, 0.48f }, { 0.54f, 0.46f }, { 0.36f, 0.80f }, { 0.66f, 0.78f } };
        lines = { { 0, 2 }, { 2, 1 }, { 0, 3 }, { 1, 5 }, { 3, 4 }, { 4, 5 }, { 3, 6 }, { 5, 7 } };
    }
    else if (p.variant == 1)   // the Plough (Ursa Major)
    {
        s = { { 0.15f, 0.35f }, { 0.30f, 0.32f }, { 0.42f, 0.38f }, { 0.52f, 0.45f }, { 0.55f, 0.62f }, { 0.75f, 0.65f }, { 0.78f, 0.46f } };
        lines = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 4 }, { 4, 5 }, { 5, 6 }, { 6, 3 } };
    }
    else                        // Cassiopeia
    {
        s = { { 0.15f, 0.40f }, { 0.32f, 0.60f }, { 0.48f, 0.42f }, { 0.65f, 0.62f }, { 0.85f, 0.38f } };
        lines = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 4 } };
    }

    const float reveal = juce::jlimit (0.0f, 1.0f, p.t / 4.0f);
    juce::Path pth;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if ((float) i / (float) lines.size() > reveal)
            break;
        pth.startNewSubPath (f.X (s[(size_t) lines[i].first].x), f.Y (s[(size_t) lines[i].first].y));
        pth.lineTo (f.X (s[(size_t) lines[i].second].x), f.Y (s[(size_t) lines[i].second].y));
    }
    glowStroke (g, pth, k.accent, 0.8f, 0.4f + 0.3f * p.mid);

    for (size_t i = 0; i < s.size(); ++i)
        glow (g, { f.X (s[i].x), f.Y (s[i].y) }, f.H() * (0.025f + 0.015f * p.snare), k.light.withAlpha (0.9f));
}

void orrery (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const auto c = juce::Point<float> (f.X (0.5f), f.Y (0.5f));

    if (p.variant == 0)
    {
        glow (g, c, f.H() * 0.06f, k.warm.withAlpha (0.9f));
        for (int i = 1; i <= 5; ++i)
        {
            const float r = f.H() * 0.075f * (float) i;
            g.setColour (k.light.withAlpha (0.25f));
            g.drawEllipse (c.x - r, c.y - r * 0.45f, r * 2.0f, r * 0.9f, 0.7f);
            const float a = p.t * (0.9f / (float) i) + hash (p.seed, i) * twoPi;
            const juce::Point<float> pl (c.x + std::cos (a) * r, c.y + std::sin (a) * r * 0.45f);
            g.setColour (k.light.withAlpha (0.9f));
            g.fillEllipse (pl.x - 3.0f, pl.y - 3.0f, 6.0f, 6.0f);
            g.setColour (k.light.withAlpha (0.2f));
            g.drawLine (c.x, c.y, pl.x, pl.y, 0.5f);
        }
    }
    else
    {
        // Ptolemaic deferent + epicycle: the looping path of a wandering planet
        const float r1 = f.H() * 0.28f, r2 = f.H() * 0.1f;
        juce::Path trace;
        const int steps = 240;
        for (int i = 0; i <= steps; ++i)
        {
            const float tt = p.t - (float) (steps - i) * 0.05f;
            const float x = c.x + std::cos (tt * 0.4f) * r1 + std::cos (tt * 2.2f) * r2;
            const float y = c.y + std::sin (tt * 0.4f) * r1 + std::sin (tt * 2.2f) * r2;
            if (i == 0) trace.startNewSubPath (x, y); else trace.lineTo (x, y);
        }
        glowStroke (g, trace, k.accent, 0.8f, 0.5f);
        glow (g, c, f.H() * 0.04f, k.light.withAlpha (0.8f));
        g.setColour (k.light.withAlpha (0.25f));
        g.drawEllipse (c.x - r1, c.y - r1, r1 * 2.0f, r1 * 2.0f, 0.6f);
    }
}

void manuscript (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const auto page = R (f.X (0.2f), f.Y (0.08f), f.W() * 0.6f, f.H() * 0.84f);
    g.setColour (k.haze.withAlpha (0.35f));
    g.fillRect (page);
    g.setColour (k.light.withAlpha (0.4f));
    g.drawRect (page.reduced (6.0f), 1.0f);

    // illuminated initial with interlace
    const auto init = R (page.getX() + 14.0f, page.getY() + 14.0f, page.getWidth() * 0.2f, page.getWidth() * 0.2f);
    g.setColour (k.accent.withAlpha (0.35f));
    g.fillRect (init);
    juce::Path knot;
    for (int i = 0; i < 4; ++i)
        knot.addEllipse (init.getX() + init.getWidth() * (0.15f + 0.35f * (float) (i % 2)), init.getY() + init.getHeight() * (0.15f + 0.35f * (float) (i / 2)),
                         init.getWidth() * 0.35f, init.getHeight() * 0.35f);
    glowStroke (g, knot, k.warm, 1.0f, 0.6f);

    // abstract script: wavering lines, written in over time
    const int lines = 14;
    const float written = p.t * 2.0f;
    for (int l = 0; l < lines; ++l)
    {
        const float y = page.getY() + page.getHeight() * (0.1f + 0.06f * (float) l) + (l < 4 ? 0.0f : 0.0f);
        const float x0 = page.getX() + (l < 4 ? page.getWidth() * 0.3f : page.getWidth() * 0.1f);
        const float len = page.getWidth() * (l < 4 ? 0.6f : 0.8f) * juce::jlimit (0.0f, 1.0f, written - (float) l);
        if (len <= 0.0f)
            break;
        juce::Path line;
        line.startNewSubPath (x0, y);
        for (float x = x0; x < x0 + len; x += 3.0f)
            line.lineTo (x, y + std::sin (x * 0.9f + (float) l) * 1.5f * hash (p.seed, (int) x));
        g.setColour (k.sil.withAlpha (0.7f));
        g.strokePath (line, juce::PathStrokeType (1.0f));
    }

    if (p.variant == 1)
    {
        // marginal diagram of nested spheres
        const juce::Point<float> c (page.getRight() - page.getWidth() * 0.22f, page.getBottom() - page.getHeight() * 0.2f);
        juce::Path d;
        for (int i = 1; i <= 4; ++i)
        {
            const float r = page.getWidth() * 0.03f * (float) i;
            d.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        }
        glowStroke (g, d, k.accent, 0.8f, 0.5f);
    }
}

void astrolabe (G& g, R area, const P& p)
{
    const Frame f { area };
    const auto& k = p.ink;
    const auto c = juce::Point<float> (f.X (0.5f), f.Y (0.5f));
    const float r = f.H() * 0.4f;
    juce::Path a;
    a.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    a.addEllipse (c.x - r * 0.88f, c.y - r * 0.88f, r * 1.76f, r * 1.76f);

    for (int i = 0; i < 72; ++i)
    {
        const float ang = (float) i / 72.0f * twoPi;
        const float r0 = r * (i % 6 == 0 ? 0.88f : 0.93f);
        a.startNewSubPath (c.x + std::sin (ang) * r0, c.y - std::cos (ang) * r0);
        a.lineTo (c.x + std::sin (ang) * r, c.y - std::cos (ang) * r);
    }

    const float rete = p.t * 0.08f;
    a.addCentredArc (c.x, c.y + r * 0.2f, r * 0.55f, r * 0.55f, rete, 0.0f, twoPi, true);
    for (int i = 0; i < 3; ++i)
        a.addCentredArc (c.x, c.y + r * 0.6f, r * (0.5f + 0.15f * (float) i), r * (0.5f + 0.15f * (float) i), 0.0f, -1.2f, 1.2f, true);

    glowStroke (g, a, k.accent, 0.9f, 0.45f + 0.3f * p.mid);

    const float rule = p.t * 0.15f;
    juce::Path alidade;
    alidade.startNewSubPath (c.x - std::sin (rule) * r, c.y + std::cos (rule) * r);
    alidade.lineTo (c.x + std::sin (rule) * r, c.y - std::cos (rule) * r);
    glowStroke (g, alidade, k.warm, 1.4f, 0.7f);
}
} // namespace

void addOccultScenes (std::vector<SceneSpec>& v)
{
    //            name                           category            fn                var  ambient rhythm sparse dark intense  rare
    v.push_back ({ "Face in the static",         Category::Occult,   ghostFace,        0, { 0.5f, 0.1f, 0.5f, 1.0f, 0.4f }, false });
    v.push_back ({ "Leaning face",               Category::Occult,   ghostFace,        1, { 0.4f, 0.2f, 0.4f, 1.0f, 0.5f }, false });
    v.push_back ({ "Two faces",                  Category::Occult,   ghostFace,        2, { 0.4f, 0.2f, 0.4f, 1.0f, 0.6f }, true  });
    v.push_back ({ "Figure in the static",       Category::Occult,   silhouetteStatic, 0, { 0.4f, 0.2f, 0.6f, 0.9f, 0.4f }, false });
    v.push_back ({ "Watcher in the doorway",     Category::Occult,   silhouetteStatic, 1, { 0.4f, 0.2f, 0.5f, 1.0f, 0.4f }, true  });
    v.push_back ({ "End of the corridor",        Category::Occult,   silhouetteStatic, 2, { 0.5f, 0.3f, 0.5f, 0.9f, 0.3f }, false });
    v.push_back ({ "The four elements",          Category::Occult,   alchemy,          0, { 0.4f, 0.6f, 0.3f, 0.6f, 0.5f }, false });
    v.push_back ({ "The squared circle",         Category::Occult,   alchemy,          1, { 0.5f, 0.5f, 0.4f, 0.6f, 0.4f }, false });
    v.push_back ({ "The seven metals",           Category::Occult,   alchemy,          2, { 0.4f, 0.6f, 0.3f, 0.6f, 0.5f }, false });
    v.push_back ({ "Invented sigil I",           Category::Occult,   inventedSigil,    0, { 0.3f, 0.6f, 0.3f, 0.8f, 0.6f }, false });
    v.push_back ({ "Invented sigil II",          Category::Occult,   inventedSigil,    1, { 0.3f, 0.6f, 0.3f, 0.8f, 0.6f }, false });
    v.push_back ({ "Invented sigil III",         Category::Occult,   inventedSigil,    2, { 0.3f, 0.7f, 0.2f, 0.8f, 0.7f }, false });
    v.push_back ({ "Runic line",                 Category::Occult,   alphabet,         0, { 0.3f, 0.8f, 0.3f, 0.7f, 0.6f }, false });
    v.push_back ({ "Ogham stem",                 Category::Occult,   alphabet,         1, { 0.4f, 0.6f, 0.4f, 0.6f, 0.4f }, false });
    v.push_back ({ "Futhark ring",               Category::Occult,   alphabet,         2, { 0.4f, 0.7f, 0.3f, 0.7f, 0.6f }, false });
    v.push_back ({ "Seed of life",               Category::Occult,   sacredGeometry,   0, { 0.5f, 0.6f, 0.3f, 0.5f, 0.5f }, false });
    v.push_back ({ "Vesica piscis",              Category::Occult,   sacredGeometry,   1, { 0.6f, 0.4f, 0.4f, 0.5f, 0.3f }, false });
    v.push_back ({ "Metatron's cube",            Category::Occult,   sacredGeometry,   2, { 0.4f, 0.7f, 0.2f, 0.6f, 0.7f }, false });
    v.push_back ({ "Eye in the signal",          Category::Occult,   eyes,             0, { 0.3f, 0.5f, 0.3f, 0.9f, 0.8f }, false });
    v.push_back ({ "Many eyes",                  Category::Occult,   eyes,             1, { 0.2f, 0.5f, 0.2f, 1.0f, 0.9f }, true  });
    v.push_back ({ "Number transmission",        Category::Occult,   numbers,          0, { 0.2f, 0.8f, 0.3f, 0.7f, 0.6f }, false });
    v.push_back ({ "Coordinates",                Category::Occult,   numbers,          1, { 0.3f, 0.6f, 0.4f, 0.7f, 0.4f }, true  });
    v.push_back ({ "Orion",                      Category::Occult,   constellation,    0, { 0.7f, 0.3f, 0.5f, 0.5f, 0.2f }, false });
    v.push_back ({ "The Plough",                 Category::Occult,   constellation,    1, { 0.7f, 0.3f, 0.5f, 0.5f, 0.2f }, false });
    v.push_back ({ "Cassiopeia",                 Category::Occult,   constellation,    2, { 0.7f, 0.3f, 0.5f, 0.5f, 0.2f }, false });
    v.push_back ({ "Orrery",                     Category::Occult,   orrery,           0, { 0.6f, 0.6f, 0.3f, 0.4f, 0.4f }, false });
    v.push_back ({ "The wandering planet",       Category::Occult,   orrery,           1, { 0.6f, 0.5f, 0.3f, 0.5f, 0.4f }, false });
    v.push_back ({ "Forgotten manuscript",       Category::Occult,   manuscript,       0, { 0.6f, 0.2f, 0.7f, 0.5f, 0.1f }, false });
    v.push_back ({ "Teaching of the spheres",    Category::Occult,   manuscript,       1, { 0.6f, 0.2f, 0.6f, 0.6f, 0.1f }, false });
    v.push_back ({ "Astrolabe",                  Category::Occult,   astrolabe,        0, { 0.5f, 0.6f, 0.3f, 0.5f, 0.4f }, false });
}
}

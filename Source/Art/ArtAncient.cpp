// =================================================================================================
//  ANCIENT & MYSTERIOUS
//  Historical symbols follow their documented forms (Elder Futhark, Ogham, the Wedjat eye, ankh,
//  alchemical signs, the classical labyrinth, valknut, triple spiral). All drawings are original.
// =================================================================================================
#include "ArtKit.h"
#include <algorithm>
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

const char* const runeSpecs[24] = {
    "30,100 30,0|30,40 75,10|30,65 75,35",            // fehu
    "25,100 25,0 75,35 75,100",                        // uruz
    "30,0 30,100|30,25 72,50 30,75",                   // thurisaz
    "30,100 30,0|30,0 72,28|30,30 72,58",              // ansuz
    "30,100 30,0 72,25 30,50 72,100",                  // raidho
    "70,15 30,50 70,85",                               // kaunan
    "20,0 80,100|80,0 20,100",                         // gebo
    "30,100 30,0 70,22 30,45",                         // wunjo
    "25,0 25,100|75,0 75,100|25,35 75,65",             // hagalaz
    "50,0 50,100|25,35 75,60",                         // naudiz
    "50,0 50,100",                                     // isaz
    "45,15 20,40 45,65|55,35 80,60 55,85",             // jera
    "50,0 50,100|50,0 75,20|50,100 25,80",             // eihwaz
    "30,0 30,100|30,0 70,20 70,35|30,100 70,80 70,65", // perthro
    "50,100 50,0|50,45 15,5|50,45 85,5",               // algiz
    "70,0 30,40 70,60 30,100",                         // sowilo
    "50,100 50,0|50,0 20,30|50,0 80,30",               // tiwaz
    "30,0 30,100|30,0 70,25 30,50 70,75 30,100",       // berkano
    "25,100 25,0 50,30 75,0 75,100",                   // ehwaz
    "25,100 25,0 75,45|75,100 75,0 25,45",             // mannaz
    "35,100 35,0 70,30",                               // laguz
    "50,10 80,50 50,90 20,50 50,10",                   // ingwaz
    "20,0 20,100 80,0 80,100 20,0",                    // dagaz
    "20,100 80,40 50,5 20,40 80,100",                  // othala
};

void runeGlyph (G& g, const Frame& f, int index, Rc box, float glowAmount, float width)
{
    const auto p = K::unitPath (runeSpecs[((index % 24) + 24) % 24], box);
    K::glowLine (g, p, f.ink.line, f.ink.glow, width, glowAmount, f.ink.light);
}

void backdrop (G& g, const Frame& f, float u, float v, float radius, float amount)
{
    K::halo (g, f.P (u, v), f.S() * radius, f.ink.haze.withAlpha (juce::jlimit (0.0f, 1.0f, 0.2f * amount)));
}

float lineW (const Frame& f, float k) { return juce::jmax (0.8f, f.S() * k); }

// ------------------------------------------------------------------------------------------------
void longship (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float seaV = 0.74f;

    juce::Path sea;
    for (int i = 0; i <= 90; ++i)
    {
        const float u = (float) i / 90.0f;
        const float x = f.box.getX() + u * f.W();
        const float y = f.Y (seaV) + std::sin (u * 18.0f + f.t * 1.4f) * f.H() * (0.015f + 0.04f * f.bass)
                                   + std::sin (u * 41.0f - f.t * 2.3f) * f.H() * 0.008f;
        if (i == 0) sea.startNewSubPath (x, y); else sea.lineTo (x, y);
    }
    K::glowLine (g, sea, k.line.withAlpha (0.55f), k.glow, 0.8f, 0.4f + f.bass, k.light);

    const float shipW = f.H() * 1.5f, shipH = f.H() * 0.72f;
    const float dir = f.flip ? -1.0f : 1.0f;
    // the ship rides the waveform sea in place (it no longer flies across the screen)
    const float cx = f.X (0.5f) + std::sin (f.t * 0.15f) * juce::jmin (f.W() * 0.08f, shipW * 0.3f);
    const float wl = f.Y (seaV) + std::sin (f.t * 1.3f) * f.H() * 0.02f;
    auto P = [&] (float u, float v) { return Pt (cx + dir * u * shipW, wl + v * shipH); };

    juce::Path hull;
    hull.startNewSubPath (P (-0.5f, -0.14f));
    hull.quadraticTo (P (-0.4f, 0.1f), P (-0.2f, 0.1f));
    hull.lineTo (P (0.2f, 0.1f));
    hull.quadraticTo (P (0.4f, 0.1f), P (0.5f, -0.14f));
    hull.quadraticTo (P (0.58f, -0.32f), P (0.5f, -0.36f));
    hull.quadraticTo (P (0.47f, -0.2f), P (0.44f, -0.1f));
    hull.lineTo (P (-0.44f, -0.1f));
    hull.quadraticTo (P (-0.47f, -0.2f), P (-0.5f, -0.36f));
    hull.quadraticTo (P (-0.58f, -0.32f), P (-0.5f, -0.14f));
    hull.closeSubPath();

    const float phase = f.t * (1.5f + 3.0f * f.bass);
    g.setColour (k.line.withAlpha (0.5f));
    for (int i = 0; i < 6; ++i)
    {
        const float u = -0.3f + 0.12f * (float) i;
        const float sweep = std::sin (phase + (float) i * 0.6f) * 0.05f;
        const auto a = P (u, -0.04f), b = P (u - 0.06f + sweep, 0.26f);
        g.drawLine (a.x, a.y, b.x, b.y, 0.8f);
    }

    K::silhouette (g, hull, f, 0.4f + 0.6f * f.bass);

    const auto mastTop = P (0.0f, -0.98f), mastBottom = P (0.0f, -0.1f);
    g.setColour (k.line.withAlpha (0.7f));
    g.drawLine (mastTop.x, mastTop.y, mastBottom.x, mastBottom.y, 1.0f);

    juce::Path sail;
    sail.startNewSubPath (P (-0.22f, -0.86f));
    sail.lineTo (P (0.22f, -0.86f));
    sail.quadraticTo (P (0.27f, -0.6f), P (0.22f, -0.34f));
    sail.lineTo (P (-0.22f, -0.34f));
    sail.quadraticTo (P (-0.17f, -0.6f), P (-0.22f, -0.86f));
    sail.closeSubPath();
    K::silhouette (g, sail, f, 0.3f);
    g.setColour (k.accent.withAlpha (0.55f));
    for (int s = -1; s <= 1; ++s)
    {
        const auto a = P (0.11f * (float) s, -0.85f), b = P (0.11f * (float) s + 0.02f, -0.35f);
        g.drawLine (a.x, a.y, b.x, b.y, 1.2f);
    }

    for (int i = 0; i < 7; ++i)
    {
        const auto c = P (-0.33f + 0.11f * (float) i, -0.12f);
        g.setColour (k.accent.withAlpha (0.8f));
        g.drawEllipse (c.x - shipH * 0.04f, c.y - shipH * 0.04f, shipH * 0.08f, shipH * 0.08f, 0.8f);
    }

    const auto rc = P (0.0f, -1.18f);
    runeGlyph (g, f, 4, Rc (shipH * 0.14f, shipH * 0.22f).withCentre (rc), 0.4f + 0.8f * f.bass + f.kick, 1.0f);
}

void runestone (G& g, const Frame& f)
{
    const auto& k = f.ink;
    backdrop (g, f, 0.5f, 0.45f, 0.6f, 1.0f + f.bass);

    juce::Path stone;
    stone.startNewSubPath (f.P (0.2f, 1.0f));
    stone.lineTo (f.P (0.15f, 0.3f));
    stone.quadraticTo (f.P (0.2f, 0.02f), f.P (0.5f, 0.02f));
    stone.quadraticTo (f.P (0.82f, 0.05f), f.P (0.85f, 0.3f));
    stone.lineTo (f.P (0.8f, 1.0f));
    stone.closeSubPath();
    K::silhouette (g, stone, f, 0.4f);

    juce::Path band;
    band.startNewSubPath (f.P (0.3f, 0.96f));
    band.quadraticTo (f.P (0.18f, 0.55f), f.P (0.42f, 0.25f));
    band.quadraticTo (f.P (0.6f, 0.06f), f.P (0.72f, 0.24f));
    band.quadraticTo (f.P (0.82f, 0.5f), f.P (0.7f, 0.96f));
    K::glowLine (g, band, k.accent.withAlpha (0.7f), k.glow, lineW (f, 0.006f), 0.4f, k.light);

    const float glowAmt = 0.3f + 0.9f * f.bass + 0.5f * f.kick;
    for (int i = 0; i < 3; ++i)
    {
        const auto box = Rc (f.S() * 0.12f, f.S() * 0.17f).withCentre (f.P (0.5f, 0.36f + 0.2f * (float) i));
        runeGlyph (g, f, (int) (K::hash (f.seed, i) * 24.0f), box, glowAmt, lineW (f, 0.006f));
    }

    g.setColour (k.line.withAlpha (0.35f));
    g.drawLine (f.box.getX(), f.box.getBottom() - 1.0f, f.box.getRight(), f.box.getBottom() - 1.0f, 0.8f);
}

void runeCircle (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.S() * 0.4f;
    backdrop (g, f, 0.5f, 0.5f, 0.5f, 0.6f + f.bass);

    g.setColour (k.line.withAlpha (0.25f));
    g.drawEllipse (c.x - r * 1.18f, c.y - r * 1.18f, r * 2.36f, r * 2.36f, 0.7f);
    g.drawEllipse (c.x - r * 0.82f, c.y - r * 0.82f, r * 1.64f, r * 1.64f, 0.7f);

    for (int i = 0; i < 24; ++i)
    {
        const float a = (float) i / 24.0f * twoPi + f.t * 0.04f * (0.4f + f.energy);
        const float wave = juce::jmax (0.0f, std::sin (f.t * 1.5f - (float) i * 0.6f));
        const auto box = Rc (f.S() * 0.055f, f.S() * 0.09f).withCentre ({ c.x + std::sin (a) * r, c.y - std::cos (a) * r });
        runeGlyph (g, f, i, box, 0.2f + 0.7f * wave + 0.4f * f.bass, lineW (f, 0.004f));
    }
}

void greatRune (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    backdrop (g, f, 0.5f, 0.5f, 0.55f, 0.8f + f.bass);
    const float r = f.S() * 0.46f;
    g.setColour (k.line.withAlpha (0.2f));
    g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 0.8f);

    for (int i = 0; i < 3; ++i)
    {
        const float a = f.t * (0.3f + 0.1f * (float) i) + (float) i * 2.1f;
        g.setColour (k.accent.withAlpha (0.7f));
        g.fillEllipse (c.x + std::sin (a) * r - 1.5f, c.y - std::cos (a) * r - 1.5f, 3.0f, 3.0f);
    }

    const auto box = Rc (f.S() * 0.36f, f.S() * 0.62f).withCentre (c);
    const float pulse = 1.0f + 0.04f * f.kick;
    runeGlyph (g, f, (int) (K::hash (f.seed, 1) * 24.0f), box.withSizeKeepingCentre (box.getWidth() * pulse, box.getHeight() * pulse),
               0.5f + 0.8f * f.bass, lineW (f, 0.012f));
}

void pyramidEye (G& g, const Frame& f)
{
    const auto& k = f.ink;
    backdrop (g, f, 0.5f, 0.2f, 0.5f, 0.8f + f.bass);

    juce::Path tri;
    tri.addTriangle (f.P (0.12f, 0.95f), f.P (0.88f, 0.95f), f.P (0.5f, 0.28f));
    K::silhouette (g, tri, f, 0.3f + 0.6f * f.bass);

    g.setColour (k.line.withAlpha (0.25f));
    for (int i = 1; i < 7; ++i)
    {
        const float v = 0.95f - (float) i * 0.095f;
        const float half = 0.38f * (v - 0.28f) / 0.67f;
        g.drawLine (f.X (0.5f - half), f.Y (v), f.X (0.5f + half), f.Y (v), 0.6f);
    }

    const float lift = 0.0f;   // the capstone stays seated on the pyramid
    juce::Path cap;
    cap.addTriangle (f.X (0.4f), f.Y (0.26f) + lift, f.X (0.6f), f.Y (0.26f) + lift, f.X (0.5f), f.Y (0.08f) + lift);
    K::halo (g, { f.X (0.5f), f.Y (0.19f) + lift }, f.S() * 0.18f, k.accent.withAlpha (0.25f + 0.3f * f.bass));
    K::glowLine (g, cap, k.accent, k.glow, lineW (f, 0.005f), 0.7f + f.bass, k.light);

    const float blinkT = std::fmod (f.t, 5.0f);
    const float blink = blinkT < 0.18f ? std::sin (pi * blinkT / 0.18f) : 0.0f;
    const float open = K::ease ((f.life - 0.15f) / 0.3f) * (1.0f - blink);
    const auto ec = Pt (f.X (0.5f), f.Y (0.195f) + lift);
    const float ew = f.W() * 0.07f, eh = ew * 0.45f * open;
    if (eh > 0.4f)
    {
        juce::Path almond;
        almond.startNewSubPath (ec.x - ew, ec.y);
        almond.quadraticTo (ec.x, ec.y - eh, ec.x + ew, ec.y);
        almond.quadraticTo (ec.x, ec.y + eh, ec.x - ew, ec.y);
        almond.closeSubPath();
        g.setColour (k.line);
        g.strokePath (almond, K::pen (0.8f));
        g.fillEllipse (ec.x - eh * 0.45f, ec.y - eh * 0.45f, eh * 0.9f, eh * 0.9f);
    }

    if (f.kick > 0.05f)
    {
        g.setColour (k.accent.withAlpha (0.4f * f.kick));
        for (int i = 0; i < 12; ++i)
        {
            const float a = (float) i / 12.0f * twoPi;
            g.drawLine (ec.x + std::sin (a) * ew * 1.6f, ec.y - std::cos (a) * ew * 1.6f,
                        ec.x + std::sin (a) * ew * 3.0f, ec.y - std::cos (a) * ew * 3.0f, 0.7f);
        }
    }
}

void pyramids (G& g, const Frame& f)
{
    const auto& k = f.ink;
    K::stars (g, f, f.seed, 40, 0.45f, k.line, f.hat);
    backdrop (g, f, 0.45f, 0.9f, 0.7f, 0.8f);

    g.setColour (k.sil.withMultipliedAlpha (0.8f));
    g.fillPath (K::ridge (f, f.seed, 0.92f, 0.06f, 0.0f, 30));

    const float sizes[] = { 0.62f, 0.46f, 0.3f };
    const float xs[] = { 0.38f, 0.62f, 0.8f };
    for (int i = 0; i < 3; ++i)
    {
        const float h = sizes[i], cx = xs[i];
        juce::Path lit, dark;
        lit.addTriangle (f.P (cx - h * 0.5f, 0.92f), f.P (cx, 0.92f), f.P (cx, 0.92f - h));
        dark.addTriangle (f.P (cx, 0.92f), f.P (cx + h * 0.5f, 0.92f), f.P (cx, 0.92f - h));
        g.setColour (k.sil.interpolatedWith (k.line, 0.12f + 0.15f * f.bass));
        g.fillPath (lit);
        g.setColour (k.sil);
        g.fillPath (dark);
        juce::Path edge;
        edge.startNewSubPath (f.P (cx - h * 0.5f, 0.92f));
        edge.lineTo (f.P (cx, 0.92f - h));
        edge.lineTo (f.P (cx + h * 0.5f, 0.92f));
        K::glowLine (g, edge, k.line.withAlpha (0.6f), k.glow, 0.8f, 0.3f + 0.6f * f.bass, k.light);
    }
}

void obelisk (G& g, const Frame& f)
{
    const auto& k = f.ink;
    K::halo (g, f.P (0.75f, 0.9f), f.S() * 0.6f, k.warm.withAlpha (0.3f + 0.15f * f.mid));
    g.setColour (k.line.withAlpha (0.4f));
    g.drawLine (f.box.getX(), f.Y (0.92f), f.box.getRight(), f.Y (0.92f), 0.8f);

    juce::Path shadow;
    shadow.startNewSubPath (f.P (0.38f, 0.92f));
    shadow.lineTo (f.P (0.46f, 0.92f));
    shadow.lineTo (f.P (0.05f, 1.0f));
    shadow.lineTo (f.P (0.0f, 1.0f));
    shadow.closeSubPath();
    g.setColour (k.sil.withMultipliedAlpha (0.6f));
    g.fillPath (shadow);

    juce::Path o;
    o.startNewSubPath (f.P (0.38f, 0.92f));
    o.lineTo (f.P (0.4f, 0.12f));
    o.lineTo (f.P (0.42f, 0.04f));
    o.lineTo (f.P (0.44f, 0.12f));
    o.lineTo (f.P (0.46f, 0.92f));
    o.closeSubPath();
    K::silhouette (g, o, f, 0.4f + 0.5f * f.bass);
    K::halo (g, f.P (0.42f, 0.06f), f.S() * 0.06f, k.accent.withAlpha (0.4f + 0.4f * f.bass));
}

void stoneCircle (G& g, const Frame& f)
{
    const auto& k = f.ink;
    K::halo (g, f.P (0.5f, 0.55f), f.S() * 0.9f, k.warm.withAlpha (0.22f + 0.1f * f.mid));
    g.setColour (k.line.withAlpha (0.25f));
    g.drawEllipse (f.X (0.1f), f.Y (0.5f), f.W() * 0.8f, f.H() * 0.35f, 0.7f);

    struct Stone { float u, v; int i; };
    std::vector<Stone> st;
    for (int i = 0; i < 11; ++i)
    {
        const float a = (float) i / 11.0f * twoPi + 0.3f;
        st.push_back ({ 0.5f + 0.38f * std::sin (a), 0.68f - 0.15f * std::cos (a), i });
    }
    std::sort (st.begin(), st.end(), [] (const Stone& a, const Stone& b) { return a.v < b.v; });

    for (const auto& s : st)
    {
        const float scale = 0.55f + (s.v - 0.53f) * 2.2f;
        const float h = f.H() * 0.42f * scale * (0.8f + 0.4f * K::hash (f.seed, s.i));
        const float w = h * 0.36f;
        juce::Path p;
        p.addRoundedRectangle (f.X (s.u) - w * 0.5f, f.Y (s.v) - h, w, h, w * 0.2f);
        K::silhouette (g, p, f, 0.2f + 0.4f * f.bass);
    }
}

void ziggurat (G& g, const Frame& f)
{
    const auto& k = f.ink;
    K::moon (g, f.P (0.85f, 0.15f), f.S() * 0.06f, k.line, 1.0f);
    float v = 0.95f;
    for (int t = 0; t < 4; ++t)
    {
        const float half = 0.42f - 0.09f * (float) t, h = 0.16f;
        juce::Path tier;
        tier.addRectangle (f.X (0.5f - half), f.Y (v - h), f.W() * half * 2.0f, f.H() * h);
        K::silhouette (g, tier, f, 0.2f + 0.4f * f.bass);
        v -= h;
    }
    juce::Path stair;
    stair.startNewSubPath (f.P (0.46f, 0.95f));
    stair.lineTo (f.P (0.49f, v));
    stair.startNewSubPath (f.P (0.54f, 0.95f));
    stair.lineTo (f.P (0.51f, v));
    K::glowLine (g, stair, k.line.withAlpha (0.6f), k.glow, 0.8f, 0.3f, k.light);
    K::halo (g, f.P (0.5f, v - 0.04f), f.S() * 0.1f, k.warm.withAlpha (0.6f + 0.3f * f.mid));
    g.setColour (k.warm);
    g.fillRect (f.X (0.495f), f.Y (v - 0.06f), f.W() * 0.01f + 1.0f, f.H() * 0.06f);
}

void torii (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float water = 0.8f;
    K::halo (g, f.P (0.5f, 0.38f), f.S() * 0.32f, k.accent.withAlpha (0.35f));

    auto gate = [&] (float alpha)
    {
        juce::Path p;
        p.addRectangle (f.X (0.33f), f.Y (0.24f), f.W() * 0.035f, f.H() * (water - 0.24f));
        p.addRectangle (f.X (0.635f), f.Y (0.24f), f.W() * 0.035f, f.H() * (water - 0.24f));
        p.addRectangle (f.X (0.27f), f.Y (0.32f), f.W() * 0.46f, f.H() * 0.03f);
        juce::Path top;
        top.startNewSubPath (f.P (0.2f, 0.17f));
        top.quadraticTo (f.P (0.5f, 0.24f), f.P (0.8f, 0.17f));
        top.lineTo (f.P (0.78f, 0.23f));
        top.quadraticTo (f.P (0.5f, 0.28f), f.P (0.22f, 0.23f));
        top.closeSubPath();
        p.addPath (top);
        g.setColour (k.sil.withMultipliedAlpha (alpha));
        g.fillPath (p);
        K::glowLine (g, p, k.line.withAlpha (0.6f * alpha), k.glow, 0.7f, 0.3f * alpha, k.light);
    };

    gate (1.0f);
    {
        G::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::scale (1.0f, -0.45f, 0.0f, f.Y (water)));
        gate (0.25f);
    }
    g.setColour (k.line.withAlpha (0.35f));
    g.drawLine (f.box.getX(), f.Y (water), f.box.getRight(), f.Y (water), 0.7f);
    K::fog (g, f, 0.72f, 0.12f, k.haze.withAlpha (0.25f + 0.1f * f.flow));
}

void pagoda (G& g, const Frame& f)
{
    const auto& k = f.ink;
    juce::Path mountain;
    mountain.startNewSubPath (f.P (0.0f, 1.0f));
    mountain.quadraticTo (f.P (0.35f, 0.62f), f.P (0.5f, 0.55f));
    mountain.quadraticTo (f.P (0.65f, 0.62f), f.P (1.0f, 1.0f));
    mountain.closeSubPath();
    K::silhouette (g, mountain, f, 0.2f);

    float v = 0.55f;
    for (int t = 0; t < 5; ++t)
    {
        const float half = 0.13f - 0.018f * (float) t, h = 0.065f;
        juce::Path tier;
        tier.addRectangle (f.X (0.5f - half * 0.55f), f.Y (v - h), f.W() * half * 1.1f, f.H() * h);
        tier.startNewSubPath (f.P (0.5f - half * 1.2f, v - h - 0.01f));
        tier.quadraticTo (f.P (0.5f - half * 0.5f, v - h + 0.012f), f.P (0.5f, v - h - 0.05f));
        tier.quadraticTo (f.P (0.5f + half * 0.5f, v - h + 0.012f), f.P (0.5f + half * 1.2f, v - h - 0.01f));
        tier.lineTo (f.P (0.5f, v - h - 0.02f));
        tier.closeSubPath();
        K::silhouette (g, tier, f, 0.25f);
        if (K::hash (f.seed, t) > 0.45f)
            K::halo (g, f.P (0.5f, v - h * 0.5f), f.S() * 0.03f, k.warm.withAlpha (0.7f));
        v -= h + 0.035f;
    }
    g.setColour (k.line.withAlpha (0.6f));
    g.drawLine (f.X (0.5f), f.Y (v + 0.03f), f.X (0.5f), f.Y (v - 0.04f), 1.0f);
    K::fog (g, f, 0.78f, 0.18f, k.haze.withAlpha (0.3f));
}

void ruinedArch (G& g, const Frame& f)
{
    const auto& k = f.ink;
    juce::Path shaft;
    shaft.startNewSubPath (f.P (0.4f, 0.0f));
    shaft.lineTo (f.P (0.5f, 0.0f));
    shaft.lineTo (f.P (0.62f, 0.95f));
    shaft.lineTo (f.P (0.38f, 0.95f));
    shaft.closeSubPath();
    juce::ColourGradient sg (k.haze.withAlpha (0.25f), 0.0f, f.box.getY(), k.haze.withAlpha (0.02f), 0.0f, f.Y (0.95f), false);
    g.setGradientFill (sg);
    g.fillPath (shaft);

    juce::Path arch;
    arch.addRectangle (f.X (0.3f), f.Y (0.2f), f.W() * 0.4f, f.H() * 0.75f);
    arch.startNewSubPath (f.P (0.38f, 0.95f));
    arch.lineTo (f.P (0.38f, 0.5f));
    arch.addCentredArc (f.X (0.5f), f.Y (0.5f), f.W() * 0.12f, f.W() * 0.12f, 0.0f, -pi * 0.5f, pi * 0.5f, false);
    arch.lineTo (f.P (0.62f, 0.95f));
    arch.closeSubPath();
    arch.setUsingNonZeroWinding (false);
    K::silhouette (g, arch, f, 0.3f);

    g.setColour (k.line.withAlpha (0.35f));
    for (int i = 0; i < 5; ++i)
    {
        juce::Path vine;
        const float u = 0.3f + 0.1f * (float) i;
        vine.startNewSubPath (f.P (u, 0.2f));
        vine.quadraticTo (f.P (u + 0.02f * std::sin (f.t + (float) i), 0.3f), f.P (u + 0.01f, 0.32f + 0.12f * K::hash (f.seed, i)));
        g.strokePath (vine, K::pen (0.7f));
    }
    g.drawLine (f.box.getX(), f.Y (0.95f), f.box.getRight(), f.Y (0.95f), 0.7f);
}

void brokenColumns (G& g, const Frame& f)
{
    const auto& k = f.ink;
    g.setColour (k.line.withAlpha (0.35f));
    g.drawLine (f.box.getX(), f.Y (0.95f), f.box.getRight(), f.Y (0.95f), 0.8f);

    for (int i = 0; i < 5; ++i)
    {
        const float u = 0.12f + 0.19f * (float) i;
        const bool broken = K::hash (f.seed, i) > 0.55f;
        const float top = broken ? 0.35f + 0.3f * K::hash (f.seed, i + 9) : 0.15f;
        juce::Path col;
        col.addRectangle (f.X (u - 0.035f), f.Y (top), f.W() * 0.07f, f.H() * (0.95f - top));
        if (! broken)
            col.addRectangle (f.X (u - 0.05f), f.Y (top - 0.04f), f.W() * 0.1f, f.H() * 0.04f);
        K::silhouette (g, col, f, 0.2f + 0.4f * f.bass);
        g.setColour (k.line.withAlpha (0.25f));
        for (int fl = -1; fl <= 1; ++fl)
            g.drawLine (f.X (u + 0.015f * (float) fl), f.Y (top + 0.02f), f.X (u + 0.015f * (float) fl), f.Y (0.94f), 0.5f);
    }

    juce::Path drum;
    drum.addEllipse (f.X (0.62f), f.Y (0.86f), f.W() * 0.06f, f.H() * 0.09f);
    K::silhouette (g, drum, f, 0.2f);
}

void desertDoor (G& g, const Frame& f)
{
    const auto& k = f.ink;
    g.setColour (k.sil.withMultipliedAlpha (0.7f));
    g.fillPath (K::ridge (f, f.seed, 0.8f, 0.08f, 0.0f, 40));

    const float light = 0.6f + 0.4f * f.mid;
    juce::Path spill;
    spill.startNewSubPath (f.P (0.44f, 0.78f));
    spill.lineTo (f.P (0.56f, 0.78f));
    spill.lineTo (f.P (0.75f, 1.0f));
    spill.lineTo (f.P (0.25f, 1.0f));
    spill.closeSubPath();
    juce::ColourGradient sg (k.warm.withAlpha (0.35f * light), 0.0f, f.Y (0.78f), k.warm.withAlpha (0.0f), 0.0f, f.box.getBottom(), false);
    g.setGradientFill (sg);
    g.fillPath (spill);

    K::halo (g, f.P (0.5f, 0.55f), f.S() * 0.4f, k.warm.withAlpha (0.2f * light));
    const auto door = Rc (f.X (0.44f) - (f.flip ? f.W() * 0.12f : 0.0f), f.Y (0.3f), f.W() * 0.12f, f.H() * 0.48f);
    g.setColour (k.warm.interpolatedWith (k.line, 0.4f).withAlpha (0.85f));
    g.fillRect (door);
    g.setColour (k.sil);
    g.drawRect (door.expanded (2.0f), 2.5f);

    g.setColour (k.line.withAlpha (0.35f));
    for (int i = 0; i < 6; ++i)
        g.fillEllipse (f.X (0.5f + 0.02f * (float) (i % 2)) - 1.0f, f.Y (0.82f + 0.03f * (float) i), 2.0f, 1.5f);
}

void elementsSigns (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const char* specs[] = { "50,10 90,85 10,85 50,10", "10,15 90,15 50,90 10,15",
                            "50,10 90,85 10,85 50,10|22,58 78,58", "10,15 90,15 50,90 10,15|22,42 78,42" };
    const auto c = f.box.getCentre();
    const float r = f.S() * 0.27f;
    for (int i = 0; i < 4; ++i)
    {
        const float a = (float) i / 4.0f * twoPi + f.t * 0.04f + pi * 0.25f;
        const auto box = Rc (f.S() * 0.2f, f.S() * 0.2f).withCentre ({ c.x + std::sin (a) * r, c.y - std::cos (a) * r });
        K::glowLine (g, K::unitPath (specs[i], box), k.line, k.glow, lineW (f, 0.006f), 0.4f + 0.6f * f.bass, k.light);
    }
    g.setColour (k.line.withAlpha (0.25f));
    g.drawEllipse (c.x - r * 1.5f, c.y - r * 1.5f, r * 3.0f, r * 3.0f, 0.8f);
}

void squaredCircle (G& g, const Frame& f)
{
    // after Michael Maier, Atalanta Fugiens (1617)
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.42f;
    juce::Path p;
    p.addEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    p.addTriangle (c.x, c.y - R, c.x + R * 0.866f, c.y + R * 0.5f, c.x - R * 0.866f, c.y + R * 0.5f);
    const float sq = R * 0.43f;
    p.addRectangle (c.x - sq, c.y - sq + R * 0.05f, sq * 2.0f, sq * 2.0f);
    p.addEllipse (c.x - sq * 0.7f, c.y - sq * 0.7f + R * 0.05f, sq * 1.4f, sq * 1.4f);
    K::glowLine (g, p, k.line, k.glow, lineW (f, 0.005f), 0.4f + 0.6f * f.bass, k.light);
    K::halo (g, { c.x, c.y + R * 0.05f }, sq * 0.6f, k.accent.withAlpha (0.25f + 0.3f * f.kick));
}

void planetaryMetals (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const char* specs[] = {
        "70,15 50,10 35,25 30,50 35,75 50,90 70,85 55,70 50,50 55,30 70,15",
        "32,12 40,22 60,22 68,12|50,63 50,95|36,80 64,80",
        "50,57 50,95|32,78 68,78",
        "56,44 85,15|62,15 85,15 85,38",
        "25,30 40,18 50,30 28,70 85,70|65,40 65,95",
        "35,5 35,60|20,20 50,20|35,45 50,35 66,45 58,72 70,92",
    };
    const float cxs[] = { 0, 50, 50, 40, 0, 0 };
    const float cys[] = { 0, 45, 35, 60, 0, 0 };
    const float rs[]  = { 0, 17, 21, 22, 0, 0 };

    const auto c = f.box.getCentre();
    juce::Path sun;
    sun.addEllipse (c.x - f.S() * 0.07f, c.y - f.S() * 0.07f, f.S() * 0.14f, f.S() * 0.14f);
    sun.addEllipse (c.x - 1.5f, c.y - 1.5f, 3.0f, 3.0f);
    K::glowLine (g, sun, k.warm, k.glow, lineW (f, 0.006f), 0.6f + f.bass, k.light);

    for (int i = 0; i < 6; ++i)
    {
        const float a = (float) i / 6.0f * twoPi - f.t * 0.03f;
        const auto box = Rc (f.S() * 0.15f, f.S() * 0.15f).withCentre ({ c.x + std::sin (a) * f.S() * 0.33f, c.y - std::cos (a) * f.S() * 0.33f });
        auto p = K::unitPath (specs[i], box);
        if (rs[i] > 0.0f)
        {
            const float cr = rs[i] / 100.0f * box.getWidth();
            p.addEllipse (box.getX() + cxs[i] / 100.0f * box.getWidth() - cr, box.getY() + cys[i] / 100.0f * box.getHeight() - cr, cr * 2.0f, cr * 2.0f);
        }
        const float wave = juce::jmax (0.0f, std::sin (f.t * 0.9f - (float) i));
        K::glowLine (g, p, k.line, k.glow, lineW (f, 0.005f), 0.3f + 0.6f * wave + 0.3f * f.bass, k.light);
    }
}

void ogham (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float x = f.X (0.5f), len = f.S() * 0.12f;
    juce::Path p;
    p.startNewSubPath (x, f.Y (0.98f));
    p.lineTo (x, f.Y (0.02f));

    const int letters = 10;
    const int shown = juce::jmin (letters, (int) (f.t * 0.8f) + 1);
    float y = f.Y (0.94f);
    for (int i = 0; i < shown && y > f.Y (0.06f); ++i)
    {
        const int aicme = (int) (K::hash (f.seed, i) * 4.0f);
        const int count = 1 + (int) (K::hash (f.seed, i + 50) * 5.0f);
        for (int s = 0; s < count; ++s)
        {
            const float sy = y - (float) s * len * 0.28f;
            if (aicme == 0)      { p.startNewSubPath (x, sy); p.lineTo (x - len, sy); }
            else if (aicme == 1) { p.startNewSubPath (x, sy); p.lineTo (x + len, sy); }
            else if (aicme == 2) { p.startNewSubPath (x - len * 0.7f, sy + len * 0.3f); p.lineTo (x + len * 0.7f, sy - len * 0.3f); }
            else                 { p.startNewSubPath (x - len * 0.3f, sy); p.lineTo (x + len * 0.3f, sy); }
        }
        y -= (float) count * len * 0.28f + len * 0.55f;
    }
    K::glowLine (g, p, k.line, k.glow, lineW (f, 0.008f), 0.4f + 0.6f * f.mid, k.light);
}

void wedjat (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto box = f.box;
    juce::Path p = K::unitPath ("8,22 30,12 60,10 92,18|10,45 30,32 55,30 80,40 92,45|10,45 35,55 60,56 80,50 92,45"
                                "|55,56 55,80 62,92 72,90 74,82 68,78|40,56 36,75 30,95", box);
    p.addEllipse (box.getX() + box.getWidth() * 0.43f, box.getY() + box.getHeight() * 0.32f, box.getWidth() * 0.12f, box.getHeight() * 0.22f);
    K::halo (g, { box.getX() + box.getWidth() * 0.49f, box.getY() + box.getHeight() * 0.43f }, f.S() * 0.35f, k.haze.withAlpha (0.2f + 0.2f * f.bass));
    K::glowLine (g, p, k.line, k.glow, lineW (f, 0.012f), 0.5f + 0.7f * f.bass, k.light);
}

void ankh (G& g, const Frame& f)
{
    const auto& k = f.ink;
    juce::Path p;
    p.addEllipse (f.X (0.32f), f.Y (0.02f), f.W() * 0.36f, f.H() * 0.32f);
    p.startNewSubPath (f.P (0.05f, 0.4f));
    p.lineTo (f.P (0.95f, 0.4f));
    p.startNewSubPath (f.P (0.5f, 0.34f));
    p.lineTo (f.P (0.5f, 0.98f));
    K::halo (g, f.P (0.5f, 0.4f), f.S() * 0.6f, k.haze.withAlpha (0.18f + 0.2f * f.bass));
    K::glowLine (g, p, k.line, k.glow, lineW (f, 0.03f), 0.5f + 0.6f * f.bass, k.light);
}

void scarab (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto sun = f.P (0.5f, 0.16f);
    K::halo (g, sun, f.S() * 0.25f, k.warm.withAlpha (0.4f + 0.3f * f.bass));
    g.setColour (k.warm.withAlpha (0.85f));
    g.fillEllipse (sun.x - f.S() * 0.09f, sun.y - f.S() * 0.09f, f.S() * 0.18f, f.S() * 0.18f);

    juce::Path body;
    body.addEllipse (f.X (0.32f), f.Y (0.42f), f.W() * 0.36f, f.H() * 0.5f);
    body.addEllipse (f.X (0.42f), f.Y (0.33f), f.W() * 0.16f, f.H() * 0.12f);
    K::silhouette (g, body, f, 0.4f);

    juce::Path legs;
    legs.startNewSubPath (f.P (0.5f, 0.45f)); legs.lineTo (f.P (0.5f, 0.9f));
    legs.startNewSubPath (f.P (0.42f, 0.36f)); legs.lineTo (f.P (0.4f, 0.24f));
    legs.startNewSubPath (f.P (0.58f, 0.36f)); legs.lineTo (f.P (0.6f, 0.24f));
    for (int s = -1; s <= 1; s += 2)
        for (int i = 0; i < 3; ++i)
        {
            const float v = 0.55f + 0.13f * (float) i;
            legs.startNewSubPath (f.P (0.5f + 0.17f * (float) s, v));
            legs.lineTo (f.P (0.5f + 0.3f * (float) s, v + 0.06f));
        }
    K::glowLine (g, legs, k.line.withAlpha (0.8f), k.glow, lineW (f, 0.008f), 0.4f, k.light);
}

void ouroboros (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.S() * 0.36f, w = f.S() * 0.06f;
    const float rot = f.t * 0.08f;
    juce::Path body;
    body.addCentredArc (c.x, c.y, r, r, rot, 0.25f, twoPi - 0.05f, true);
    g.setColour (k.sil);
    g.strokePath (body, K::pen (w));
    K::glowLine (g, body, k.line.withAlpha (0.7f), k.glow, lineW (f, 0.004f), 0.4f + 0.6f * f.bass, k.light);

    g.setColour (k.line.withAlpha (0.4f));
    for (int i = 0; i < 40; ++i)
    {
        const float a = rot + 0.3f + (float) i / 40.0f * (twoPi - 0.4f);
        const float r0 = r - w * 0.3f, r1 = r + w * 0.3f;
        g.drawLine (c.x + std::sin (a) * r0, c.y - std::cos (a) * r0, c.x + std::sin (a + 0.04f) * r1, c.y - std::cos (a + 0.04f) * r1, 0.5f);
    }

    const float ha = rot + 0.22f;
    const Pt head (c.x + std::sin (ha) * r, c.y - std::cos (ha) * r);
    juce::Path hp;
    hp.addEllipse (head.x - w * 0.8f, head.y - w * 0.6f, w * 1.6f, w * 1.2f);
    K::silhouette (g, hp, f, 0.5f);
    g.setColour (k.accent);
    g.fillEllipse (head.x - 1.5f, head.y - 1.5f, 3.0f, 3.0f);
}

void valknut (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.S() * 0.3f;
    juce::Path p;
    for (int i = 0; i < 3; ++i)
    {
        const float a = (float) i / 3.0f * twoPi + f.t * 0.03f;
        const Pt o (c.x + std::sin (a) * r * 0.35f, c.y - std::cos (a) * r * 0.35f);
        p.addPath (K::polygon (o, 3, r, a + pi));
    }
    K::glowLine (g, p, k.line, k.glow, lineW (f, 0.008f), 0.4f + 0.7f * f.bass, k.light);
}

void triskele (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.18f;
    juce::Path p;
    for (int arm = 0; arm < 3; ++arm)
    {
        const float base = (float) arm / 3.0f * twoPi + f.t * 0.05f;
        const Pt o (c.x + std::sin (base) * R, c.y - std::cos (base) * R);
        for (int i = 0; i <= 80; ++i)
        {
            const float th = (float) i / 80.0f * 2.6f * twoPi;
            const float rr = R * (1.0f - (float) i / 80.0f);
            const Pt q (o.x + std::sin (base + pi + th) * rr, o.y - std::cos (base + pi + th) * rr);
            if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
        }
    }
    K::glowLine (g, p, k.line, k.glow, lineW (f, 0.006f), 0.4f + 0.6f * f.bass, k.light);
}

void labyrinth (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.45f;
    juce::Path p;
    for (int i = 1; i <= 7; ++i)
    {
        const float r = R * (float) i / 7.0f;
        const float gap = 0.25f + 0.05f * (float) (i % 2);
        const float start = (i % 2 == 0 ? pi : 0.0f) + gap;
        p.addCentredArc (c.x, c.y, r, r, 0.0f, start, start + twoPi - gap * 2.0f, true);
    }
    for (int i = 1; i < 7; ++i)
    {
        const float r0 = R * (float) i / 7.0f, r1 = R * (float) (i + 1) / 7.0f;
        const float a = (i % 2 == 0) ? pi + 0.25f : 0.25f;
        p.startNewSubPath (c.x + std::sin (a) * r0, c.y - std::cos (a) * r0);
        p.lineTo (c.x + std::sin (a) * r1, c.y - std::cos (a) * r1);
    }
    K::glowLine (g, p, k.line.withAlpha (0.85f), k.glow, lineW (f, 0.004f), 0.3f + 0.5f * f.mid, k.light);

    // a traveller of light walking the rings
    const float ring = 1.0f + std::fmod (f.t * 0.15f, 6.0f);
    const float a = f.t * 0.9f;
    const float rr = R * ring / 7.0f;
    K::halo (g, { c.x + std::sin (a) * rr, c.y - std::cos (a) * rr }, f.S() * 0.04f, k.accent.withAlpha (0.9f));
}
} // namespace

void addAncient (std::vector<Spec>& v)
{
    const int hero = flagHero, symbol = flagHero | flagNoFlip;
    //            name                    category            fn               zone            size  aspect  life           quiet bass  melod energ   flags
    v.push_back ({ "Viking longship",      Category::Ancient, longship,        Zone::BandMid,  0.42f, 1.0f, 16.0f, 24.0f, { 0.3f, 0.9f, 0.6f, 0.5f }, hero });
    v.push_back ({ "Runestone",            Category::Ancient, runestone,       Zone::Ground,   0.62f, 0.55f, 12.0f, 22.0f, { 0.5f, 0.9f, 0.3f, 0.3f }, symbol });
    v.push_back ({ "Circle of runes",      Category::Ancient, runeCircle,      Zone::Centre,   0.78f, 1.0f, 12.0f, 20.0f, { 0.3f, 0.7f, 0.4f, 0.8f }, symbol | flagSpin });
    v.push_back ({ "The great rune",       Category::Ancient, greatRune,       Zone::Centre,   0.6f,  0.7f, 9.0f,  16.0f, { 0.7f, 0.8f, 0.3f, 0.2f }, symbol });
    v.push_back ({ "Pyramid and eye",      Category::Ancient, pyramidEye,      Zone::Centre,   0.72f, 1.3f, 12.0f, 20.0f, { 0.3f, 0.7f, 0.3f, 0.9f }, hero });
    v.push_back ({ "Three pyramids",       Category::Ancient, pyramids,        Zone::Ground,   0.5f,  2.4f, 14.0f, 24.0f, { 0.6f, 0.5f, 0.4f, 0.3f }, hero });
    v.push_back ({ "Obelisk at dusk",      Category::Ancient, obelisk,         Zone::Ground,   0.7f,  0.9f, 12.0f, 20.0f, { 0.7f, 0.4f, 0.5f, 0.2f }, hero });
    v.push_back ({ "Stone circle",         Category::Ancient, stoneCircle,     Zone::Ground,   0.5f,  2.2f, 14.0f, 24.0f, { 0.6f, 0.6f, 0.4f, 0.3f }, hero });
    v.push_back ({ "Ziggurat",             Category::Ancient, ziggurat,        Zone::Ground,   0.55f, 1.5f, 12.0f, 20.0f, { 0.5f, 0.6f, 0.3f, 0.4f }, hero });
    v.push_back ({ "Torii in the mist",    Category::Ancient, torii,           Zone::Ground,   0.62f, 1.4f, 14.0f, 26.0f, { 0.9f, 0.3f, 0.7f, 0.1f }, hero });
    v.push_back ({ "Pagoda on the peak",   Category::Ancient, pagoda,          Zone::Ground,   0.75f, 1.0f, 14.0f, 26.0f, { 0.8f, 0.3f, 0.6f, 0.1f }, hero });
    v.push_back ({ "Ruined arch",          Category::Ancient, ruinedArch,      Zone::Ground,   0.62f, 1.2f, 12.0f, 22.0f, { 0.7f, 0.4f, 0.6f, 0.2f }, hero });
    v.push_back ({ "Broken columns",       Category::Ancient, brokenColumns,   Zone::Ground,   0.5f,  2.0f, 12.0f, 22.0f, { 0.6f, 0.5f, 0.4f, 0.3f }, hero });
    v.push_back ({ "Door in the desert",   Category::Ancient, desertDoor,      Zone::Ground,   0.6f,  1.3f, 12.0f, 20.0f, { 0.8f, 0.3f, 0.6f, 0.1f }, hero });
    v.push_back ({ "The four elements",    Category::Ancient, elementsSigns,   Zone::Centre,   0.62f, 1.0f, 10.0f, 18.0f, { 0.4f, 0.6f, 0.4f, 0.7f }, symbol | flagSpin });
    v.push_back ({ "The squared circle",   Category::Ancient, squaredCircle,   Zone::Centre,   0.62f, 1.0f, 10.0f, 18.0f, { 0.5f, 0.6f, 0.4f, 0.5f }, symbol | flagSpin });
    v.push_back ({ "The seven metals",     Category::Ancient, planetaryMetals, Zone::Centre,   0.7f,  1.0f, 10.0f, 18.0f, { 0.4f, 0.5f, 0.5f, 0.6f }, symbol | flagSpin });
    v.push_back ({ "Ogham inscription",    Category::Ancient, ogham,           Zone::Centre,   0.75f, 0.35f, 10.0f, 16.0f, { 0.7f, 0.4f, 0.5f, 0.2f }, symbol });
    v.push_back ({ "Wedjat eye",           Category::Ancient, wedjat,          Zone::Centre,   0.36f, 1.6f, 9.0f,  16.0f, { 0.4f, 0.8f, 0.3f, 0.6f }, symbol });
    v.push_back ({ "Ankh",                 Category::Ancient, ankh,            Zone::Centre,   0.5f,  0.62f, 8.0f, 14.0f, { 0.6f, 0.6f, 0.4f, 0.3f }, symbol });
    v.push_back ({ "Scarab and sun",       Category::Ancient, scarab,          Zone::Centre,   0.5f,  1.0f, 9.0f,  16.0f, { 0.5f, 0.6f, 0.4f, 0.4f }, symbol });
    v.push_back ({ "Ouroboros",            Category::Ancient, ouroboros,       Zone::Centre,   0.62f, 1.0f, 10.0f, 18.0f, { 0.5f, 0.7f, 0.5f, 0.5f }, symbol | flagSpin });
    v.push_back ({ "Valknut",              Category::Ancient, valknut,         Zone::Centre,   0.5f,  1.0f, 9.0f,  16.0f, { 0.3f, 0.8f, 0.3f, 0.7f }, symbol | flagSpin });
    v.push_back ({ "Triple spiral",        Category::Ancient, triskele,        Zone::Centre,   0.5f,  1.0f, 9.0f,  16.0f, { 0.6f, 0.5f, 0.6f, 0.4f }, symbol | flagSpin });
    v.push_back ({ "Labyrinth",            Category::Ancient, labyrinth,       Zone::Centre,   0.7f,  1.0f, 12.0f, 22.0f, { 0.7f, 0.4f, 0.6f, 0.3f }, symbol | flagSpin });
}
}

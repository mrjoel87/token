// =================================================================================================
//  GEOMETRIC & ABSTRACT - sacred geometry, impossible figures and pure motion
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
constexpr float pi = juce::MathConstants<float>::pi;
constexpr float twoPi = juce::MathConstants<float>::twoPi;

float lineW (const Frame& f, float k) { return juce::jmax (0.8f, f.S() * k); }
float glowFor (const Frame& f) { return 0.35f + 0.6f * f.bass + 0.3f * f.kick; }

void circles (G& g, const Frame& f, int rings, float scale, bool connect)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.S() * scale;
    const float rot = f.t * 0.04f;
    std::vector<Pt> centres { c };
    for (int ring = 1; ring <= rings; ++ring)
        for (int i = 0; i < 6; ++i)
        {
            const float a = rot + (float) i / 6.0f * twoPi;
            centres.push_back ({ c.x + std::sin (a) * r * (float) ring, c.y - std::cos (a) * r * (float) ring });
        }

    juce::Path p;
    for (const auto& pt : centres)
        p.addEllipse (pt.x - r, pt.y - r, r * 2.0f, r * 2.0f);
    if (connect)
        for (size_t i = 0; i < centres.size(); ++i)
            for (size_t j = i + 1; j < centres.size(); ++j)
            {
                p.startNewSubPath (centres[i]);
                p.lineTo (centres[j]);
            }
    K::glowLine (g, p, k.line.withAlpha (0.8f), k.glow, lineW (f, 0.003f), glowFor (f), k.light);
}

void seedOfLife (G& g, const Frame& f)   { circles (g, f, 1, 0.16f, false); }
void flowerOfLife (G& g, const Frame& f) { circles (g, f, 2, 0.11f, false); }
void metatron (G& g, const Frame& f)     { circles (g, f, 2, 0.1f, true); }

void vesica (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.S() * 0.3f;
    const float sep = r * (0.5f + 0.5f * std::sin (f.t * 0.3f) * 0.5f + 0.25f);
    juce::Path p;
    p.addEllipse (c.x - sep - r, c.y - r, r * 2.0f, r * 2.0f);
    p.addEllipse (c.x + sep - r, c.y - r, r * 2.0f, r * 2.0f);
    K::glowLine (g, p, k.line, k.glow, lineW (f, 0.004f), glowFor (f), k.light);
    K::halo (g, c, r * 0.5f, k.accent.withAlpha (0.25f + 0.3f * f.bass));
}

void hexagram (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.S() * 0.42f;
    const float rot = f.t * (0.05f + 0.2f * f.energy);
    juce::Path p;
    p.addPath (K::polygon (c, 3, r, rot));
    p.addPath (K::polygon (c, 3, r, rot + pi));
    p.addPath (K::polygon (c, 6, r * 0.5f, -rot));
    p.addEllipse (c.x - r * 1.08f, c.y - r * 1.08f, r * 2.16f, r * 2.16f);
    K::glowLine (g, p, k.line, k.glow, lineW (f, 0.004f), glowFor (f), k.light);
}

void icosahedron (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float s = f.S() * 0.3f * (1.0f + 0.05f * f.kick);
    const float phi = 1.618034f;
    const float verts[12][3] = { { -1, phi, 0 }, { 1, phi, 0 }, { -1, -phi, 0 }, { 1, -phi, 0 },
                                 { 0, -1, phi }, { 0, 1, phi }, { 0, -1, -phi }, { 0, 1, -phi },
                                 { phi, 0, -1 }, { phi, 0, 1 }, { -phi, 0, -1 }, { -phi, 0, 1 } };
    const float ay = f.t * (0.2f + 0.4f * f.energy), ax = f.t * 0.13f;
    Pt v[12];
    for (int i = 0; i < 12; ++i)
    {
        const float x = verts[i][0], y = verts[i][1], z = verts[i][2];
        const float x1 = x * std::cos (ay) + z * std::sin (ay), z1 = -x * std::sin (ay) + z * std::cos (ay);
        const float y1 = y * std::cos (ax) - z1 * std::sin (ax);
        v[i] = { c.x + x1 * s * 0.55f, c.y + y1 * s * 0.55f };
    }
    juce::Path p;
    for (int i = 0; i < 12; ++i)
        for (int j = i + 1; j < 12; ++j)
        {
            const float dx = verts[i][0] - verts[j][0], dy = verts[i][1] - verts[j][1], dz = verts[i][2] - verts[j][2];
            if (std::abs (dx * dx + dy * dy + dz * dz - 4.0f) < 0.1f)
            {
                p.startNewSubPath (v[i]);
                p.lineTo (v[j]);
            }
        }
    K::glowLine (g, p, k.line.withAlpha (0.85f), k.glow, 0.9f, glowFor (f), k.light);
}

void torus (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.28f, r = f.S() * 0.11f;
    const float tilt = 0.9f + 0.2f * std::sin (f.t * 0.2f);
    const float spin = f.t * (0.2f + 0.3f * f.mid);
    juce::Path p;
    for (int i = 0; i < 18; ++i)
    {
        const float u = (float) i / 18.0f * twoPi + spin;
        for (int j = 0; j <= 24; ++j)
        {
            const float w = (float) j / 24.0f * twoPi;
            const float x = (R + r * std::cos (w)) * std::cos (u);
            const float y = (R + r * std::cos (w)) * std::sin (u);
            const float z = r * std::sin (w);
            const Pt q (c.x + x, c.y + y * std::cos (tilt) - z * std::sin (tilt));
            if (j == 0) p.startNewSubPath (q); else p.lineTo (q);
        }
    }
    K::glowLine (g, p, k.line.withAlpha (0.5f), k.glow, 0.6f, 0.3f + 0.4f * f.bass, k.light);
}

void lissajous (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float a = 3.0f, b = 2.0f + std::round (f.energy * 3.0f);
    const float delta = f.t * (0.2f + 0.5f * f.mid);
    const float amp = f.S() * 0.42f * (1.0f + 0.05f * f.kick);
    juce::Path p;
    for (int i = 0; i <= 600; ++i)
    {
        const float t = (float) i / 600.0f * twoPi;
        const Pt q (c.x + std::sin (a * t + delta) * amp, c.y + std::sin (b * t) * amp);
        if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
    }
    K::glowLine (g, p, k.line.withAlpha (0.8f), k.glow, 0.8f, glowFor (f), k.light);
}

void spirograph (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = 5.0f, r = 3.0f, d = 2.0f + std::sin (f.t * 0.2f) * 1.5f;
    const float scale = f.S() * 0.42f / (R - r + d);
    const float drawn = juce::jmin (1.0f, f.t / 6.0f);
    juce::Path p;
    const int steps = 900;
    for (int i = 0; i <= (int) ((float) steps * drawn); ++i)
    {
        const float t = (float) i / (float) steps * twoPi * 3.0f;
        const Pt q (c.x + ((R - r) * std::cos (t) + d * std::cos ((R - r) / r * t)) * scale,
                    c.y + ((R - r) * std::sin (t) - d * std::sin ((R - r) / r * t)) * scale);
        if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
    }
    K::glowLine (g, p, k.line.withAlpha (0.8f), k.glow, 0.8f, glowFor (f), k.light);
}

void goldenSpiral (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    juce::Path p;
    const float b = 0.30634f;   // golden spiral growth
    const float rot = f.t * 0.1f;
    for (int i = 0; i <= 400; ++i)
    {
        const float th = (float) i / 400.0f * 4.0f * pi;
        const float r = f.S() * 0.008f * std::exp (b * th);
        const Pt q (c.x + std::cos (th + rot) * r, c.y + std::sin (th + rot) * r);
        if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
    }
    K::glowLine (g, p, k.line, k.glow, lineW (f, 0.004f), glowFor (f), k.light);
    for (int i = 0; i < 6; ++i)
    {
        const float th = (float) i * pi * 0.5f + rot;
        const float r = f.S() * 0.008f * std::exp (b * th);
        g.setColour (k.accent.withAlpha (0.7f));
        g.fillEllipse (c.x + std::cos (th) * r - 2.0f, c.y + std::sin (th) * r - 2.0f, 4.0f, 4.0f);
    }
}

void ripples (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    for (int i = 0; i < 9; ++i)
    {
        const float ph = std::fmod ((float) i / 9.0f + f.t * (0.06f + 0.1f * f.bass), 1.0f);
        const float r = f.S() * 0.48f * ph;
        g.setColour (k.line.withAlpha (0.6f * (1.0f - ph)));
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 0.8f + 1.5f * f.kick * (1.0f - ph));
    }
    K::halo (g, c, f.S() * 0.06f, k.accent.withAlpha (0.6f + 0.4f * f.kick));
}

void tesseract (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float s = f.S() * 0.22f;
    const float aw = f.t * (0.25f + 0.4f * f.energy), ay = f.t * 0.17f;
    Pt v[16];
    for (int i = 0; i < 16; ++i)
    {
        float x = (i & 1) ? 1.0f : -1.0f, y = (i & 2) ? 1.0f : -1.0f, z = (i & 4) ? 1.0f : -1.0f, w = (i & 8) ? 1.0f : -1.0f;
        const float x1 = x * std::cos (aw) - w * std::sin (aw), w1 = x * std::sin (aw) + w * std::cos (aw);
        const float y1 = y * std::cos (ay) - z * std::sin (ay), z1 = y * std::sin (ay) + z * std::cos (ay);
        const float p4 = 2.5f / (2.5f - w1 * 0.6f);
        const float p3 = 3.0f / (3.0f + z1 * 0.5f);
        v[i] = { c.x + x1 * s * p4 * p3, c.y + y1 * s * p4 * p3 };
    }
    juce::Path p;
    for (int i = 0; i < 16; ++i)
        for (int bit = 0; bit < 4; ++bit)
        {
            const int j = i ^ (1 << bit);
            if (j > i)
            {
                p.startNewSubPath (v[i]);
                p.lineTo (v[j]);
            }
        }
    K::glowLine (g, p, k.line.withAlpha (0.8f), k.glow, 0.8f, glowFor (f), k.light);
}

void phyllotaxis (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const int n = 320;
    const float spacing = f.S() * 0.42f / std::sqrt ((float) n);
    const float golden = pi * (3.0f - std::sqrt (5.0f));
    const int shown = juce::jmin (n, (int) (f.t * 60.0f));
    for (int i = 0; i < shown; ++i)
    {
        const float a = (float) i * golden + f.t * 0.05f;
        const float r = spacing * std::sqrt ((float) i);
        const float s = 1.0f + 2.0f * (1.0f - (float) i / (float) n) + 1.5f * f.kick * (float) (i % 8 == 0);
        g.setColour ((i % 13 == 0 ? k.accent : k.line).withAlpha (0.4f + 0.5f * (float) i / (float) n));
        g.fillEllipse (c.x + std::cos (a) * r - s * 0.5f, c.y + std::sin (a) * r - s * 0.5f, s, s);
    }
}

void penroseTriangle (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.42f, w = R * 0.22f;
    const float rot = f.t * 0.03f;
    juce::Path p;
    for (int i = 0; i < 3; ++i)
    {
        const float a0 = rot + (float) i / 3.0f * twoPi, a1 = rot + (float) (i + 1) / 3.0f * twoPi;
        const Pt o0 (c.x + std::sin (a0) * R, c.y - std::cos (a0) * R);
        const Pt o1 (c.x + std::sin (a1) * R, c.y - std::cos (a1) * R);
        const Pt i0 (c.x + std::sin (a0) * (R - w * 1.7f), c.y - std::cos (a0) * (R - w * 1.7f));
        const Pt i1 (c.x + std::sin (a1) * (R - w * 1.7f), c.y - std::cos (a1) * (R - w * 1.7f));
        p.startNewSubPath (o0);
        p.lineTo (o1);
        p.startNewSubPath (i0);
        p.lineTo (i1);
        p.startNewSubPath (o0);
        p.lineTo (i0.x + (i1.x - i0.x) * 0.25f, i0.y + (i1.y - i0.y) * 0.25f);
    }
    K::glowLine (g, p, k.line, k.glow, lineW (f, 0.005f), glowFor (f), k.light);
}

void standingWaves (G& g, const Frame& f)
{
    const auto& k = f.ink;
    for (int l = 0; l < 9; ++l)
    {
        juce::Path p;
        const float v = 0.1f + 0.1f * (float) l;
        const float harmonic = 1.0f + (float) (l % 4);
        for (int i = 0; i <= 80; ++i)
        {
            const float u = (float) i / 80.0f;
            const float y = f.Y (v) + std::sin (u * pi * harmonic) * std::sin (f.t * (1.0f + 0.5f * (float) l) * (0.6f + f.mid))
                                      * f.H() * 0.04f * (0.5f + f.bass);
            if (i == 0) p.startNewSubPath (f.box.getX() + u * f.W(), y); else p.lineTo (f.box.getX() + u * f.W(), y);
        }
        g.setColour (k.line.withAlpha (0.25f + 0.04f * (float) l));
        g.strokePath (p, K::pen (0.8f));
    }
}

void inventedSigil (G& g, const Frame& f)
{
    // invented, fictional - generated from the seed, not taken from any tradition
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.4f;
    const int n = 5 + (int) (K::hash (f.seed, 0) * 4.0f);
    juce::Path p;
    p.addEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    for (int i = 0; i <= n; ++i)
    {
        const int idx = (int) (K::hash (f.seed, i + 1) * 12.0f);
        const float a = (float) idx / 12.0f * twoPi;
        const Pt v (c.x + std::sin (a) * R * 0.8f, c.y - std::cos (a) * R * 0.8f);
        if (i == 0) p.startNewSubPath (v); else p.lineTo (v);
        if (i == 0 || i == n)
            p.addEllipse (v.x - R * 0.05f, v.y - R * 0.05f, R * 0.1f, R * 0.1f);
        if (i > 0) p.startNewSubPath (v);
    }
    K::glowLine (g, p, k.line, k.glow, lineW (f, 0.005f), (0.3f + 0.6f * f.bass) * K::ease (f.t / 3.0f), k.light);
}
} // namespace

void addGeometric (std::vector<Spec>& v)
{
    const int sym = flagHero | flagNoFlip;
    //            name                    category              fn               zone            size  aspect  life           quiet bass  melod energ   flags
    v.push_back ({ "Seed of life",         Category::Geometric, seedOfLife,      Zone::Centre,   0.62f, 1.0f, 10.0f, 18.0f, { 0.6f, 0.5f, 0.6f, 0.4f }, sym });
    v.push_back ({ "Flower of life",       Category::Geometric, flowerOfLife,    Zone::Centre,   0.72f, 1.0f, 10.0f, 18.0f, { 0.5f, 0.5f, 0.6f, 0.5f }, sym });
    v.push_back ({ "Metatron's cube",      Category::Geometric, metatron,        Zone::Centre,   0.72f, 1.0f, 10.0f, 18.0f, { 0.2f, 0.7f, 0.4f, 0.9f }, sym });
    v.push_back ({ "Vesica piscis",        Category::Geometric, vesica,          Zone::Centre,   0.55f, 1.6f, 10.0f, 18.0f, { 0.7f, 0.4f, 0.6f, 0.2f }, sym });
    v.push_back ({ "Hexagram",             Category::Geometric, hexagram,        Zone::Centre,   0.62f, 1.0f, 10.0f, 18.0f, { 0.3f, 0.8f, 0.3f, 0.8f }, sym });
    v.push_back ({ "Icosahedron",          Category::Geometric, icosahedron,     Zone::Centre,   0.6f,  1.0f, 10.0f, 18.0f, { 0.2f, 0.7f, 0.4f, 0.9f }, sym });
    v.push_back ({ "Torus",                Category::Geometric, torus,           Zone::Centre,   0.55f, 1.4f, 10.0f, 18.0f, { 0.4f, 0.6f, 0.7f, 0.5f }, sym });
    v.push_back ({ "Lissajous rosette",    Category::Geometric, lissajous,       Zone::Centre,   0.62f, 1.0f, 9.0f,  16.0f, { 0.2f, 0.6f, 0.5f, 1.0f }, sym });
    v.push_back ({ "Spirograph",           Category::Geometric, spirograph,      Zone::Centre,   0.62f, 1.0f, 10.0f, 18.0f, { 0.5f, 0.3f, 0.8f, 0.5f }, sym });
    v.push_back ({ "Golden spiral",        Category::Geometric, goldenSpiral,    Zone::Centre,   0.62f, 1.2f, 10.0f, 18.0f, { 0.7f, 0.3f, 0.7f, 0.3f }, sym });
    v.push_back ({ "Ripples",              Category::Geometric, ripples,         Zone::Centre,   0.62f, 1.0f, 8.0f,  14.0f, { 0.3f, 1.0f, 0.4f, 0.6f }, sym });
    v.push_back ({ "Tesseract",            Category::Geometric, tesseract,       Zone::Centre,   0.62f, 1.0f, 10.0f, 18.0f, { 0.1f, 0.7f, 0.3f, 1.0f }, sym });
    v.push_back ({ "Sunflower spiral",     Category::Geometric, phyllotaxis,     Zone::Centre,   0.62f, 1.0f, 10.0f, 18.0f, { 0.7f, 0.3f, 0.8f, 0.3f }, sym });
    v.push_back ({ "Impossible triangle",  Category::Geometric, penroseTriangle, Zone::Centre,   0.6f,  1.0f, 10.0f, 18.0f, { 0.4f, 0.6f, 0.4f, 0.6f }, sym });
    v.push_back ({ "Standing waves",       Category::Geometric, standingWaves,   Zone::Full,     1.0f,  1.0f, 10.0f, 18.0f, { 0.4f, 0.8f, 0.7f, 0.6f }, sym });
    v.push_back ({ "Invented sigil",       Category::Geometric, inventedSigil,   Zone::Centre,   0.6f,  1.0f, 9.0f,  16.0f, { 0.3f, 0.7f, 0.3f, 0.7f }, sym });
}
}

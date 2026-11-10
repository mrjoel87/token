// =================================================================================================
//  MANDALAS, SACRED GEOMETRY & THE NIGHT MOTORWAY
//  Every figure here is driven by the music: rings turn in eased steps on the beat (beatStep),
//  petals and radii follow the spectrum, kicks pulse, the bass swells. The engine adds a global
//  kick/bass pulse and a beat-stepped turn on top, so inner rings counter-rotate against it.
//  All designs are original; "inspired by" figures are not copies of specific sacred diagrams.
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
float glowFor (const Frame& f) { return 0.35f + 0.6f * f.bass + 0.35f * f.kick; }
float frac (float x) { return x - std::floor (x); }
int beatIndex (const Frame& f) { return (int) std::floor (f.beats); }

Pt onCircle (Pt c, float r, float a) { return { c.x + std::sin (a) * r, c.y - std::cos (a) * r }; }

juce::Path starPolygon (Pt c, int n, int skip, float r, float rot)
{
    juce::Path p;
    for (int i = 0; i <= n; ++i)
    {
        const auto v = onCircle (c, r, rot + twoPi * (float) ((i * skip) % n) / (float) n);
        if (i == 0) p.startNewSubPath (v); else p.lineTo (v);
    }
    p.closeSubPath();
    return p;
}

// ------------------------------------------------------------------------------------------------
void mandalaRings (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.47f;
    juce::Path p;
    for (int ring = 0; ring < 4; ++ring)
    {
        const int n = 6 + 4 * ring;
        const float r = R * (0.3f + 0.22f * (float) ring);
        const float dir = (ring % 2 == 0) ? 1.0f : -1.0f;
        const float rot = dir * K::beatStep (f.beats, 1.0f + (float) ring) * pi / (float) n;
        const float len = R * 0.12f * (1.0f + 0.7f * K::spectrumAt (f, (float) ring / 3.0f));
        for (int j = 0; j < n; ++j)
        {
            const float a = rot + (float) j / (float) n * twoPi;
            juce::Path petal;
            petal.addEllipse (-len * 0.22f, -len * 0.5f, len * 0.44f, len);
            petal.applyTransform (juce::AffineTransform::rotation (a).translated (onCircle (c, r, a).x, onCircle (c, r, a).y));
            p.addPath (petal);
        }
        const float rr = r - len * 0.65f;
        p.addEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f);
    }
    p.addPath (starPolygon (c, 12, 5, R * 0.18f, K::beatStep (f.beats, 2.0f) * pi / 6.0f));
    K::glowLine (g, p, k.line.withAlpha (0.85f), k.glow, lineW (f, 0.0025f), glowFor (f), k.light);
    K::halo (g, c, R * 0.12f, k.accent.withAlpha (0.4f + 0.5f * f.kick));
}

void lotusMandala (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.46f;
    const int counts[] = { 8, 12, 16 };
    for (int layer = 2; layer >= 0; --layer)
    {
        const int n = counts[layer];
        const float len = R * (0.35f + 0.28f * (float) layer) * (0.85f + 0.3f * f.bass);
        const float rot = (float) layer * pi / (float) n + K::beatStep (f.beats, 2.0f) * pi / (float) n * (layer == 1 ? -1.0f : 1.0f);
        juce::Path p;
        for (int j = 0; j < n; ++j)
        {
            const float a = rot + (float) j / (float) n * twoPi;
            const float half = pi / (float) n * 0.9f;
            p.startNewSubPath (c);
            p.quadraticTo (onCircle (c, len * 0.7f, a - half), onCircle (c, len, a));
            p.quadraticTo (onCircle (c, len * 0.7f, a + half), c);
        }
        g.setColour ((layer == 1 ? k.accent : k.glow).withAlpha (0.07f));
        g.fillPath (p);
        K::glowLine (g, p, k.line.withAlpha (0.5f + 0.15f * (float) layer), k.glow, lineW (f, 0.0025f), glowFor (f) * 0.7f, k.light);
    }
    K::halo (g, c, R * 0.1f, k.accent.withAlpha (0.6f));
}

void interlockingTriangles (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.46f;
    juce::Path all, lit;
    const int litIndex = beatIndex (f) % 9;
    for (int i = 0; i < 9; ++i)
    {
        const bool up = i % 2 == 0;
        const float s = R * (1.0f - (float) i * 0.085f);
        const Pt o (c.x, c.y + (up ? -1.0f : 1.0f) * R * 0.04f * (float) (i % 3));
        const auto tri = K::polygon (o, 3, s, up ? 0.0f : pi);
        all.addPath (tri);
        if (i == litIndex)
            lit.addPath (tri);
    }
    all.addEllipse (c.x - R * 1.04f, c.y - R * 1.04f, R * 2.08f, R * 2.08f);
    K::glowLine (g, all, k.line.withAlpha (0.65f), k.glow, lineW (f, 0.0025f), glowFor (f) * 0.6f, k.light);
    K::glowLine (g, lit, k.accent, k.glow, lineW (f, 0.004f), 0.8f + f.kick, k.light);
}

void starPolygons (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.46f;
    const int ns[] = { 7, 9, 5, 11 };
    const int skips[] = { 3, 4, 2, 5 };
    const float sizes[] = { 1.0f, 0.72f, 0.48f, 0.28f };
    for (int i = 0; i < 4; ++i)
    {
        const float dir = (i % 2 == 0) ? 1.0f : -1.0f;
        const float rot = dir * K::beatStep (f.beats, 1.0f + (float) i * 0.5f) * twoPi / (float) (ns[i] * 2);
        const auto p = starPolygon (c, ns[i], skips[i], R * sizes[i] * (1.0f + 0.05f * f.bass), rot);
        K::glowLine (g, p, (i == 2 ? k.accent : k.line).withAlpha (0.85f), k.glow, lineW (f, 0.003f), glowFor (f), k.light);
    }
}

void nestedSquares (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.66f;
    const float twist = 0.12f + 0.1f * std::sin (K::beatStep (f.beats, 4.0f) * pi * 0.5f) + 0.06f * f.bass;
    juce::Path p;
    float s = R;
    for (int i = 0; i < 16; ++i)
    {
        p.addPath (K::polygon (c, 4, s, (float) i * twist + K::beatStep (f.beats, 2.0f) * 0.05f));
        s *= 0.86f;
    }
    K::glowLine (g, p, k.line.withAlpha (0.75f), k.glow, lineW (f, 0.0025f), glowFor (f), k.light);
}

void roseCurve (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.46f * (0.85f + 0.2f * f.bass);
    const int bar = (int) std::floor (f.beats / 4.0f);
    const float k1 = (float) (3 + (bar % 5)), k2 = (float) (3 + ((bar + 1) % 5));
    const float m = K::ease ((frac (f.beats / 4.0f) - 0.6f) / 0.4f);
    juce::Path p;
    for (int i = 0; i <= 720; ++i)
    {
        const float th = (float) i / 720.0f * twoPi * 2.0f;
        const float r = R * ((1.0f - m) * std::cos (k1 * th) + m * std::cos (k2 * th));
        const Pt q (c.x + std::cos (th) * r, c.y + std::sin (th) * r);
        if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
    }
    K::glowLine (g, p, k.line.withAlpha (0.85f), k.glow, lineW (f, 0.003f), glowFor (f), k.light);
}

void epitrochoid (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R0 = 5.0f, r0 = 1.0f + (float) (f.seed % 3u);
    const float d = 2.0f + 1.2f * std::sin (f.t * 0.25f) + 0.6f * f.bass;
    const float scale = f.S() * 0.46f / (R0 + r0 + d);
    const float drawn = juce::jmin (1.0f, 0.15f + f.t / 8.0f);
    juce::Path p;
    const int steps = 900;
    for (int i = 0; i <= (int) ((float) steps * drawn); ++i)
    {
        const float th = (float) i / (float) steps * twoPi * r0;
        const Pt q (c.x + ((R0 + r0) * std::cos (th) - d * std::cos ((R0 + r0) / r0 * th)) * scale,
                    c.y + ((R0 + r0) * std::sin (th) - d * std::sin ((R0 + r0) / r0 * th)) * scale);
        if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
    }
    K::glowLine (g, p, k.line.withAlpha (0.8f), k.glow, lineW (f, 0.0025f), glowFor (f), k.light);
}

void staveCompass (G& g, const Frame& f)
{
    // an original eight-armed stave, in the spirit of Icelandic magical staves (not a copy of one)
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.46f;
    const int lit = beatIndex (f) % 8;
    juce::Path all, glowArm;
    for (int arm = 0; arm < 8; ++arm)
    {
        const float a = (float) arm / 8.0f * twoPi;
        juce::Path p;
        const auto inner = onCircle (c, R * 0.16f, a), tip = onCircle (c, R * 0.92f, a);
        p.startNewSubPath (inner);
        p.lineTo (tip);
        const auto across = [&] (float along, float half)
        {
            const auto m = onCircle (c, R * along, a);
            const Pt side (std::cos (a) * half, std::sin (a) * half);
            p.startNewSubPath (m - side);
            p.lineTo (m + side);
        };
        switch (arm % 4)
        {
            case 0: across (0.92f, R * 0.09f);
                    p.startNewSubPath (onCircle (c, R * 0.92f, a - 0.09f)); p.lineTo (onCircle (c, R * 0.8f, a - 0.09f));
                    p.startNewSubPath (onCircle (c, R * 0.92f, a + 0.09f)); p.lineTo (onCircle (c, R * 0.8f, a + 0.09f)); break;
            case 1: { const auto e = onCircle (c, R * 0.86f, a); p.addEllipse (e.x - R * 0.05f, e.y - R * 0.05f, R * 0.1f, R * 0.1f); across (0.62f, R * 0.07f); } break;
            case 2: p.startNewSubPath (onCircle (c, R * 0.7f, a)); p.lineTo (onCircle (c, R * 0.86f, a - 0.12f));
                    p.startNewSubPath (onCircle (c, R * 0.7f, a)); p.lineTo (onCircle (c, R * 0.86f, a + 0.12f)); break;
            default: across (0.5f, R * 0.06f); across (0.75f, R * 0.08f); break;
        }
        all.addPath (p);
        if (arm == lit)
            glowArm.addPath (p);
    }
    all.addEllipse (c.x - R * 0.16f, c.y - R * 0.16f, R * 0.32f, R * 0.32f);
    K::glowLine (g, all, k.line.withAlpha (0.8f), k.glow, lineW (f, 0.004f), glowFor (f) * 0.7f, k.light);
    K::glowLine (g, glowArm, k.accent, k.glow, lineW (f, 0.006f), 0.8f + f.kick, k.light);
}

void triquetra (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.42f * (1.0f + 0.04f * f.bass);
    juce::Path p;
    for (int i = 0; i < 3; ++i)
    {
        const float a0 = (float) i / 3.0f * twoPi, a1 = (float) (i + 1) / 3.0f * twoPi;
        const auto v0 = onCircle (c, R * 0.5f, a0), v1 = onCircle (c, R * 0.5f, a1);
        const float mid = (a0 + a1) * 0.5f;
        p.startNewSubPath (v0);
        p.quadraticTo (onCircle (c, R * 1.45f, mid), v1);
        p.quadraticTo (onCircle (c, R * 0.1f, mid + pi), v0);
    }
    p.addEllipse (c.x - R * 0.62f, c.y - R * 0.62f, R * 1.24f, R * 1.24f);
    K::glowLine (g, p, k.line, k.glow, lineW (f, 0.006f), glowFor (f), k.light);
}

void borromean (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.S() * 0.27f;
    const float rot = K::beatStep (f.beats, 2.0f) * twoPi / 12.0f;
    for (int i = 0; i < 3; ++i)
    {
        const auto o = onCircle (c, r * 0.6f * (1.0f + 0.1f * f.bass), rot + (float) i / 3.0f * twoPi);
        juce::Path ring;
        ring.addEllipse (o.x - r, o.y - r, r * 2.0f, r * 2.0f);
        K::glowLine (g, ring, (i == beatIndex (f) % 3 ? k.accent : k.line).withAlpha (0.9f), k.glow, lineW (f, 0.006f), glowFor (f), k.light);
    }
}

void dotMandala (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.47f;
    const float wave = frac (f.beats) * 8.0f;     // a ring of light travels outwards on each beat
    for (int ring = 1; ring <= 8; ++ring)
    {
        const int n = 6 * ring;
        const float r = R * (float) ring / 8.0f;
        const float closeness = juce::jmax (0.0f, 1.0f - std::abs ((float) ring - wave));
        const float rot = (ring % 2 == 0 ? 1.0f : -1.0f) * K::beatStep (f.beats, 2.0f) * pi / (float) n;
        for (int j = 0; j < n; ++j)
        {
            const auto p = onCircle (c, r, rot + (float) j / (float) n * twoPi);
            const float s = 1.2f + 1.8f * closeness + 1.5f * f.hat * K::hash (f.seed, ring * 40 + j);
            g.setColour ((closeness > 0.5f ? k.accent : k.line).withAlpha (0.35f + 0.6f * closeness));
            g.fillEllipse (p.x - s, p.y - s, s * 2.0f, s * 2.0f);
        }
    }
}

void spectrumMandala (G& g, const Frame& f)
{
    // the shape is the music: radius at each angle is a spectrum band, folded twelve times
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.47f;
    for (int layer = 0; layer < 2; ++layer)
    {
        juce::Path p;
        const int folds = layer == 0 ? 12 : 8;
        const float rot = (layer == 0 ? 1.0f : -1.0f) * K::beatStep (f.beats, 2.0f) * pi / (float) folds;
        for (int i = 0; i <= 360; ++i)
        {
            const float th = (float) i / 360.0f * twoPi;
            float u = frac ((th / twoPi) * (float) folds);
            u = u < 0.5f ? u * 2.0f : (1.0f - u) * 2.0f;                  // mirrored within each fold
            const float level = K::spectrumAt (f, layer == 0 ? u * 0.6f : 0.4f + u * 0.6f);
            const float r = R * (layer == 0 ? (0.3f + 0.7f * level) : (0.15f + 0.45f * level));
            const auto q = onCircle (c, r, th + rot);
            if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
        }
        p.closeSubPath();
        g.setColour ((layer == 0 ? k.glow : k.accent).withAlpha (0.08f));
        g.fillPath (p);
        K::glowLine (g, p, (layer == 0 ? k.line : k.accent).withAlpha (0.85f), k.glow, lineW (f, 0.0025f), glowFor (f), k.light);
    }
}

void spectrumRings (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.47f;
    for (int i = 0; i < 16; ++i)
    {
        const float level = K::spectrumAt (f, (float) i / 15.0f);
        const float r = R * (0.12f + 0.88f * (float) i / 15.0f);
        const float start = (i % 2 == 0 ? 1.0f : -1.0f) * K::beatStep (f.beats, 1.0f + (float) (i % 4)) * pi / 4.0f;
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, r, r, 0.0f, start, start + 0.3f + level * (twoPi - 0.3f), true);
        g.setColour ((level > 0.6f ? k.accent : k.line).withAlpha (0.25f + 0.7f * level));
        g.strokePath (arc, K::pen (0.8f + 2.0f * level));
    }
}

void merkaba (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float s = f.S() * 0.27f;
    const float ay = f.t * 0.35f + K::beatStep (f.beats, 2.0f) * 0.2f, ax = 0.5f + 0.2f * std::sin (f.t * 0.2f);
    const float verts[8][3] = { { 1, 1, 1 }, { 1, -1, -1 }, { -1, 1, -1 }, { -1, -1, 1 },
                                { -1, -1, -1 }, { -1, 1, 1 }, { 1, -1, 1 }, { 1, 1, -1 } };
    Pt v[8];
    for (int i = 0; i < 8; ++i)
    {
        const float x = verts[i][0], y = verts[i][1], z = verts[i][2];
        const float x1 = x * std::cos (ay) + z * std::sin (ay), z1 = -x * std::sin (ay) + z * std::cos (ay);
        const float y1 = y * std::cos (ax) - z1 * std::sin (ax);
        v[i] = { c.x + x1 * s, c.y + y1 * s };
    }
    juce::Path a, b;
    for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 4; ++j)
        {
            a.startNewSubPath (v[i]);     a.lineTo (v[j]);
            b.startNewSubPath (v[i + 4]); b.lineTo (v[j + 4]);
        }
    K::glowLine (g, a, k.line, k.glow, lineW (f, 0.004f), glowFor (f), k.light);
    K::glowLine (g, b, k.accent, k.glow, lineW (f, 0.004f), glowFor (f), k.light);
}

void triangleSpiral (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    float s = f.S() * 0.5f;
    const float twist = 0.09f + 0.06f * std::sin (K::beatStep (f.beats, 4.0f) * pi * 0.5f) + 0.04f * f.bass;
    for (int i = 0; i < 26; ++i)
    {
        const auto tri = K::polygon (c, 3, s, (float) i * twist);
        g.setColour ((i % 5 == 0 ? k.accent : k.line).withAlpha (0.25f + 0.65f * (1.0f - (float) i / 26.0f)));
        g.strokePath (tri, K::pen (lineW (f, 0.0025f)));
        s *= 0.9f;
    }
    K::halo (g, c, f.S() * 0.05f, k.accent.withAlpha (0.4f + 0.5f * f.kick));
}

void moireRings (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.47f;
    const float off = R * 0.09f * std::sin (f.t * 0.5f) * (1.0f + f.bass);
    for (int set = 0; set < 2; ++set)
    {
        const float cx = c.x + (set == 0 ? -off : off);
        g.setColour ((set == 0 ? k.line : k.accent).withAlpha (0.35f + 0.2f * f.kick));
        for (int i = 1; i <= 22; ++i)
        {
            const float r = R * (float) i / 22.0f;
            g.drawEllipse (cx - r, c.y - r, r * 2.0f, r * 2.0f, 0.7f);
        }
    }
}

void polygonPulse (G& g, const Frame& f)
{
    // a development of the pulsing ripples: a new hexagon is born on every beat and expands outwards
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.5f;
    const float fb = frac (f.beats);
    for (int j = 0; j < 8; ++j)
    {
        const float age = ((float) j + fb) / 8.0f;
        const float r = R * age;
        const float alpha = (1.0f - age) * (0.4f + 0.6f * (j == 0 ? 1.0f - fb : 1.0f));
        const auto hex = K::polygon (c, 6, r, (float) j * 0.12f + (float) beatIndex (f) * 0.0f);
        K::glowLine (g, hex, (j % 2 == 0 ? k.line : k.accent).withAlpha (juce::jlimit (0.0f, 1.0f, alpha)), k.glow,
                     lineW (f, 0.003f) * (1.0f + 1.5f * f.kick * (1.0f - age)), glowFor (f) * (1.0f - age), k.light);
    }
    K::halo (g, c, R * 0.08f * (1.0f + f.kick), k.accent.withAlpha (0.6f));
}

void breathingFlower (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.S() * 0.11f;
    const int pulsing = beatIndex (f) % 3;
    const float shape = std::exp (-frac (f.beats) * 3.0f);
    juce::Path calm, lit;
    calm.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    for (int ring = 1; ring <= 2; ++ring)
        for (int i = 0; i < 6 * ring; ++i)
        {
            const float a = (float) i / (float) (6 * ring) * twoPi;
            const float dist = ring == 1 ? r : (i % 2 == 0 ? r * 2.0f : r * 1.732f);
            const auto o = onCircle (c, dist, a + (ring == 2 && i % 2 == 1 ? 0.0f : 0.0f));
            const bool on = ring == pulsing || (pulsing == 0 && ring == 2 && i % 2 == 0);
            const float rr = r * (1.0f + (on ? 0.18f * shape : 0.0f));
            (on ? lit : calm).addEllipse (o.x - rr, o.y - rr, rr * 2.0f, rr * 2.0f);
        }
    calm.addEllipse (c.x - r * 3.0f, c.y - r * 3.0f, r * 6.0f, r * 6.0f);
    K::glowLine (g, calm, k.line.withAlpha (0.75f), k.glow, lineW (f, 0.003f), glowFor (f) * 0.6f, k.light);
    K::glowLine (g, lit, k.accent, k.glow, lineW (f, 0.004f), 0.6f + f.kick, k.light);
}

// ------------------------------------------------------------------------------------------------
//  THE NIGHT MOTORWAY - first-person, endless, in time with the music
// ------------------------------------------------------------------------------------------------
void motorway (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float horizon = 0.42f;
    const Pt vp (f.box.getCentreX(), f.Y (horizon));
    const float bottom = f.box.getBottom();
    const float depth = bottom - vp.y;
    const float W = f.W();

    // the glow of somewhere far ahead, and its tiny lights
    K::blob (g, vp, W * 0.45f, f.H() * 0.12f, k.haze.withAlpha (0.2f + 0.12f * f.energy));
    for (int i = 0; i < 40; ++i)
    {
        const float u = 0.12f + 0.76f * K::hash (f.seed, i);
        const float tw = 0.4f + 0.6f * std::abs (std::sin (f.t * (1.0f + 2.0f * K::hash (f.seed, i + 40)) + (float) i));
        g.setColour (k.warm.withAlpha (juce::jlimit (0.0f, 1.0f, 0.2f * tw * (0.5f + f.high))));
        g.fillRect (f.box.getX() + u * W, vp.y - f.H() * 0.015f * K::hash (f.seed, i + 80) - 1.0f, 1.5f, 1.5f);
    }

    // land and low hills, then the road
    g.setColour (k.sil);
    g.fillPath (K::ridge (f, f.seed, horizon + 0.005f, 0.05f, 0.2f, 40));

    juce::Path road;
    road.startNewSubPath (vp.x - W * 0.01f, vp.y);
    road.lineTo (vp.x + W * 0.01f, vp.y);
    road.lineTo (vp.x + W * 0.75f, bottom);
    road.lineTo (vp.x - W * 0.75f, bottom);
    road.closeSubPath();
    juce::ColourGradient tarmac (k.sil, 0.0f, vp.y, k.sil.interpolatedWith (k.line, 0.08f + 0.06f * f.kick), 0.0f, bottom, false);
    g.setGradientFill (tarmac);
    g.fillPath (road);

    juce::Path edges;
    edges.startNewSubPath (vp.x - W * 0.01f, vp.y); edges.lineTo (vp.x - W * 0.75f, bottom);
    edges.startNewSubPath (vp.x + W * 0.01f, vp.y); edges.lineTo (vp.x + W * 0.75f, bottom);
    K::glowLine (g, edges, k.line.withAlpha (0.45f), k.glow, 0.9f, 0.3f, k.light);

    // lane markings flowing towards us - one every two beats at normal speed, faster with the bass
    const float dashes = f.travel * 0.5f;
    for (int i = 0; i < 10; ++i)
    {
        const float d0 = frac (dashes + (float) i / 10.0f);
        const float d1 = juce::jmin (1.0f, d0 + 0.035f);
        const float z0 = std::pow (d0, 2.2f), z1 = std::pow (d1, 2.2f);
        for (int lane = -1; lane <= 1; lane += 2)
        {
            const Pt a (vp.x + (float) lane * W * 0.25f * z0, vp.y + depth * z0);
            const Pt b (vp.x + (float) lane * W * 0.25f * z1, vp.y + depth * z1);
            g.setColour (k.line.withAlpha (0.25f + 0.65f * z0));
            g.drawLine (a.x, a.y, b.x, b.y, 0.8f + 4.0f * z0);
        }
    }

    // lamp posts on both sides - one passes on every beat; kicks brighten the sodium light
    const float lamps = f.travel;
    for (int i = 0; i < 8; ++i)
    {
        const float d = frac (lamps * 0.5f + (float) i / 8.0f);
        const float z = std::pow (d, 2.4f);
        for (int side = -1; side <= 1; side += 2)
        {
            const float x = vp.x + (float) side * W * 0.62f * z;
            const float y = vp.y + depth * z;
            const float h = depth * 1.4f * z;
            const Pt top (x - (float) side * h * 0.16f, y - h);
            g.setColour (k.sil.interpolatedWith (k.line, 0.25f).withAlpha (0.4f + 0.6f * z));
            g.drawLine (x, y, x, y - h, 0.6f + 2.5f * z);
            g.drawLine (x, y - h, top.x, top.y, 0.6f + 2.0f * z);
            K::halo (g, top, juce::jmax (2.0f, h * 0.14f), k.warm.withAlpha (juce::jlimit (0.0f, 1.0f, (0.25f + 0.5f * z) * (1.0f + 0.8f * f.kick))));
            K::blob (g, { x - (float) side * W * 0.18f * z, y }, juce::jmax (1.0f, W * 0.12f * z), juce::jmax (0.6f, depth * 0.025f * z),
                     k.warm.withAlpha (juce::jlimit (0.0f, 1.0f, 0.12f * (1.0f + f.kick) * z)));
        }
    }

    // tail-lights ahead, drifting gently
    for (int car = 0; car < 3; ++car)
    {
        const float d = 0.1f + 0.06f * (float) car + 0.012f * std::sin (f.t * 0.3f + (float) car);
        const float z = d * d;
        const float x = vp.x + W * 0.1f * z * (car == 1 ? -1.0f : 1.0f) * 2.0f;
        const float y = vp.y + depth * z;
        for (int s = -1; s <= 1; s += 2)
            K::halo (g, { x + (float) s * W * 0.03f * (0.3f + z), y }, 2.0f + 6.0f * z, k.accent.withAlpha (0.8f));
    }
}
} // namespace

void addMandala (std::vector<Spec>& v)
{
    const int sym = flagHero | flagNoFlip;
    //            name                     category              fn                     zone          size  aspect  life           quiet bass  melod energ   flags
    v.push_back ({ "Mandala of rings",      Category::Geometric, mandalaRings,          Zone::Centre, 0.85f, 1.0f, 12.0f, 22.0f, { 0.4f, 0.7f, 0.6f, 0.8f }, sym });
    v.push_back ({ "Lotus mandala",         Category::Geometric, lotusMandala,          Zone::Centre, 0.85f, 1.0f, 12.0f, 22.0f, { 0.7f, 0.5f, 0.8f, 0.4f }, sym });
    v.push_back ({ "Nine triangles",        Category::Geometric, interlockingTriangles, Zone::Centre, 0.82f, 1.0f, 12.0f, 20.0f, { 0.3f, 0.8f, 0.4f, 0.8f }, sym });
    v.push_back ({ "Star polygons",         Category::Geometric, starPolygons,          Zone::Centre, 0.82f, 1.0f, 10.0f, 18.0f, { 0.3f, 0.7f, 0.4f, 0.9f }, sym });
    v.push_back ({ "Turning squares",       Category::Geometric, nestedSquares,         Zone::Centre, 0.82f, 1.0f, 10.0f, 18.0f, { 0.3f, 0.8f, 0.4f, 0.8f }, sym });
    v.push_back ({ "Rose curve",            Category::Geometric, roseCurve,             Zone::Centre, 0.82f, 1.0f, 12.0f, 22.0f, { 0.5f, 0.5f, 0.8f, 0.6f }, sym });
    v.push_back ({ "Epitrochoid flower",    Category::Geometric, epitrochoid,           Zone::Centre, 0.82f, 1.0f, 12.0f, 20.0f, { 0.6f, 0.4f, 0.8f, 0.5f }, sym });
    v.push_back ({ "Eight-armed stave",     Category::Geometric, staveCompass,          Zone::Centre, 0.82f, 1.0f, 10.0f, 18.0f, { 0.3f, 0.9f, 0.3f, 0.7f }, sym });
    v.push_back ({ "Triquetra",             Category::Geometric, triquetra,             Zone::Centre, 0.7f,  1.0f, 10.0f, 18.0f, { 0.6f, 0.6f, 0.6f, 0.4f }, sym });
    v.push_back ({ "Borromean rings",       Category::Geometric, borromean,             Zone::Centre, 0.72f, 1.0f, 10.0f, 18.0f, { 0.4f, 0.7f, 0.5f, 0.6f }, sym });
    v.push_back ({ "Mandala of points",     Category::Geometric, dotMandala,            Zone::Centre, 0.85f, 1.0f, 10.0f, 18.0f, { 0.4f, 0.7f, 0.5f, 0.8f }, sym });
    v.push_back ({ "Spectrum mandala",      Category::Geometric, spectrumMandala,       Zone::Centre, 0.88f, 1.0f, 12.0f, 22.0f, { 0.3f, 0.8f, 0.7f, 0.9f }, sym });
    v.push_back ({ "Rings of sound",        Category::Geometric, spectrumRings,         Zone::Centre, 0.85f, 1.0f, 10.0f, 18.0f, { 0.3f, 0.8f, 0.6f, 0.8f }, sym });
    v.push_back ({ "Star tetrahedron",      Category::Geometric, merkaba,               Zone::Centre, 0.7f,  1.0f, 10.0f, 18.0f, { 0.2f, 0.7f, 0.4f, 0.9f }, sym });
    v.push_back ({ "Triangle spiral",       Category::Geometric, triangleSpiral,        Zone::Centre, 0.82f, 1.0f, 10.0f, 18.0f, { 0.4f, 0.7f, 0.6f, 0.7f }, sym });
    v.push_back ({ "Moire rings",           Category::Geometric, moireRings,            Zone::Centre, 0.85f, 1.0f, 8.0f,  14.0f, { 0.4f, 0.9f, 0.4f, 0.6f }, sym });
    v.push_back ({ "Hexagon pulse",         Category::Geometric, polygonPulse,          Zone::Centre, 0.85f, 1.0f, 10.0f, 18.0f, { 0.2f, 1.0f, 0.3f, 0.9f }, sym });
    v.push_back ({ "Breathing flower",      Category::Geometric, breathingFlower,       Zone::Centre, 0.78f, 1.0f, 10.0f, 18.0f, { 0.5f, 0.8f, 0.6f, 0.6f }, sym });
    v.push_back ({ "Night motorway",        Category::Sentimental, motorway,            Zone::Full,   1.0f,  1.0f, 18.0f, 36.0f, { 0.5f, 0.7f, 0.6f, 0.8f }, flagHero | flagNoFlip });
}
}

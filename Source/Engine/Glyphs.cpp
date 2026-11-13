#include "Glyphs.h"
#include <array>

namespace token::engine
{
namespace
{
    using P = juce::Point<float>;
    constexpr float pi = juce::MathConstants<float>::pi;
    constexpr float twoPi = juce::MathConstants<float>::twoPi;
    const P O { 0.0f, 0.0f };

    float rnd (juce::uint32 seed, int i)
    {
        juce::uint32 x = seed * 747796405u + (juce::uint32) i * 2891336453u + 0x9e3779b9u;
        x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
        return (float) (x & 0xffffffu) / 16777216.0f;
    }

    // ------------------------------------------------------------------ 3D / 4D projection
    struct V3 { float x, y, z; };
    struct V4 { float x, y, z, w; };

    V3 rotY (V3 v, float a) { const float c = std::cos (a), s = std::sin (a); return { c * v.x + s * v.z, v.y, -s * v.x + c * v.z }; }
    V3 rotX (V3 v, float a) { const float c = std::cos (a), s = std::sin (a); return { v.x, c * v.y - s * v.z, s * v.y + c * v.z }; }

    P project (V3 v)
    {
        const float k = 3.2f / (3.2f + v.z);
        return { v.x * k * 0.62f, v.y * k * 0.62f };
    }

    V3 turn3 (V3 v, const GlyphCtx& c)
    {
        return rotX (rotY (v, c.spin), c.spin * 0.61f + 0.35f);
    }

    /** Rotates in two 4D planes (XW and YZ / ZW), then projects 4D -> 3D -> 2D. */
    P project4 (V4 v, const GlyphCtx& c)
    {
        const float a = c.spin * 0.9f, b = c.spin * 0.53f + 0.4f;
        float x = std::cos (a) * v.x - std::sin (a) * v.w;
        float w = std::sin (a) * v.x + std::cos (a) * v.w;
        float z = std::cos (b) * v.z - std::sin (b) * w;
        w = std::sin (b) * v.z + std::cos (b) * w;
        const float k = 2.4f / (2.4f - w);
        const V3 p3 { x * k * 0.55f, v.y * k * 0.55f, z * k * 0.55f };
        return project (rotX (rotY (p3, c.spin * 0.3f), 0.45f));
    }

    void lens (Sketch& s, P a, P b, float bulge, int segs = 16)
    {
        const P d = b - a;
        const P n (-d.y, d.x);
        for (int side = 0; side < 2; ++side)
        {
            const float sg = side == 0 ? 1.0f : -1.0f;
            for (int i = 0; i <= segs; ++i)
            {
                const float u = (float) i / (float) segs;
                const P p = a + d * u + n * (sg * bulge * std::sin (pi * u));
                if (i == 0) s.moveTo (p); else s.lineTo (p);
            }
        }
    }

    void arrow (Sketch& s, P from, P to, float head)
    {
        s.line (from, to);
        const float a = std::atan2 (to.x - from.x, -(to.y - from.y));
        s.moveTo (Sketch::polar (to, head, a + pi - 0.45f));
        s.lineTo (to);
        s.lineTo (Sketch::polar (to, head, a + pi + 0.45f));
    }

    // ================================================================== OCCULT
    void pentagram (Sketch& s, const GlyphCtx&)
    {
        s.circle (O, 1.0f, 64);
        s.star (O, 0.92f, 5, 2, 0.0f);
        s.circle (O, 0.36f, 32);
    }

    void hexagram (Sketch& s, const GlyphCtx&)
    {
        s.circle (O, 1.0f, 64);
        s.star (O, 0.93f, 6, 2, 0.0f);
        s.circle (O, 0.5f, 40);
    }

    void heptagram (Sketch& s, const GlyphCtx&)
    {
        s.circle (O, 1.0f, 64);
        s.polygon (O, 0.95f, 7, 0.0f);
        s.star (O, 0.95f, 7, 3, 0.0f);
    }

    void octagram (Sketch& s, const GlyphCtx& c)
    {
        s.circle (O, 1.0f, 64);
        s.star (O, 0.95f, 8, 2, c.t * 0.05f);
        s.star (O, 0.6f, 8, 3, -c.t * 0.08f);
    }

    void enneagram (Sketch& s, const GlyphCtx&)
    {
        s.circle (O, 1.0f, 64);
        s.star (O, 0.95f, 9, 4, 0.0f);
        s.star (O, 0.55f, 9, 2, pi / 9.0f);
    }

    void eyeInTriangle (Sketch& s, const GlyphCtx& c)
    {
        s.polygon ({ 0.0f, 0.12f }, 1.0f, 3, 0.0f);
        s.eye ({ 0.0f, 0.28f }, 0.4f, 0.18f * (1.0f - 0.6f * c.kick));
        s.circle ({ 0.0f, 0.28f }, 0.1f + 0.04f * c.kick, 20);
        for (int i = 0; i < 12; ++i)
        {
            const float a = twoPi * (float) i / 12.0f;
            const float len = (i % 2 == 0 ? 0.28f : 0.16f) * (0.6f + 0.6f * c.high);
            s.line (Sketch::polar ({ 0.0f, 0.12f }, 1.08f, a), Sketch::polar ({ 0.0f, 0.12f }, 1.08f + len, a));
        }
    }

    void chaosStar (Sketch& s, const GlyphCtx& c)
    {
        s.circle (O, 0.18f + 0.05f * c.kick, 24);
        for (int i = 0; i < 8; ++i)
        {
            const float a = twoPi * (float) i / 8.0f;
            arrow (s, Sketch::polar (O, 0.2f, a), Sketch::polar (O, 0.98f, a), 0.16f);
        }
    }

    void elements (Sketch& s, const GlyphCtx&)
    {
        const float r = 0.36f;
        s.polygon ({ -0.48f, -0.42f }, r, 3, 0.0f);                 // fire
        s.polygon ({ 0.48f, -0.48f }, r, 3, pi);                    // water
        s.polygon ({ -0.48f, 0.52f }, r, 3, 0.0f);                  // air
        s.line (-0.48f - r * 0.75f, 0.52f, -0.48f + r * 0.75f, 0.52f);
        s.polygon ({ 0.48f, 0.46f }, r, 3, pi);                     // earth
        s.line (0.48f - r * 0.75f, 0.46f, 0.48f + r * 0.75f, 0.46f);
    }

    void sulfur (Sketch& s, const GlyphCtx&)
    {
        s.polygon ({ 0.0f, -0.42f }, 0.5f, 3, 0.0f);
        s.line (0.0f, -0.17f, 0.0f, 0.95f);
        s.line (-0.4f, 0.32f, 0.4f, 0.32f);
        s.line (-0.4f, 0.62f, 0.4f, 0.62f);
    }

    void mercury (Sketch& s, const GlyphCtx&)
    {
        s.circle ({ 0.0f, -0.05f }, 0.3f, 32);
        s.arc ({ 0.0f, -0.62f }, 0.27f, 0.27f, pi * 0.5f, pi * 1.5f, 20);
        s.line (0.0f, 0.25f, 0.0f, 0.95f);
        s.line (-0.22f, 0.6f, 0.22f, 0.6f);
    }

    void ankh (Sketch& s, const GlyphCtx&)
    {
        s.ellipse ({ 0.0f, -0.5f }, 0.28f, 0.36f, 36);
        s.line (-0.55f, -0.06f, 0.55f, -0.06f);
        s.line (0.0f, -0.14f, 0.0f, 0.96f);
    }

    void sigil (Sketch& s, const GlyphCtx& c)
    {
        s.circle (O, 1.0f, 64);
        const juce::uint32 seed = c.seed;
        P first;
        P prev;
        for (int i = 0; i < 7; ++i)
        {
            const P p ((float) ((int) (rnd (seed, i * 2) * 3.0f) - 1) * 0.62f,
                       (float) ((int) (rnd (seed, i * 2 + 1) * 3.0f) - 1) * 0.62f);
            if (i == 0) { s.moveTo (p); first = p; } else s.lineTo (p);
            prev = p;
        }
        s.circle (first, 0.09f, 14);
        s.line (prev + P (-0.12f, 0.12f), prev + P (0.12f, -0.12f));
    }

    void runeStave (Sketch& s, const GlyphCtx& c)
    {
        s.line (0.0f, -0.95f, 0.0f, 0.95f);
        for (int i = 0; i < 5; ++i)
        {
            const float y = -0.75f + 0.36f * (float) i;
            const float len = 0.25f + 0.3f * rnd (c.seed, i);
            const int kind = (int) (rnd (c.seed, i + 10) * 3.0f);
            if (kind != 1) s.line (0.0f, y, -len, y - len * 0.7f);
            if (kind != 0) s.line (0.0f, y, len, y - len * 0.7f);
        }
        s.polygon ({ 0.0f, -0.95f }, 0.1f, 4, 0.0f);
    }

    void magicSquare (Sketch& s, const GlyphCtx&)
    {
        const float h = 0.82f, cell = h * 2.0f / 4.0f;
        for (int i = 0; i <= 4; ++i)
        {
            const float v = -h + cell * (float) i;
            s.line (-h, v, h, v);
            s.line (v, -h, v, h);
        }
        static const int square[4][4] = { { 16, 3, 2, 13 }, { 5, 10, 11, 8 }, { 9, 6, 7, 12 }, { 4, 15, 14, 1 } };
        std::array<P, 17> pos {};
        for (int r = 0; r < 4; ++r)
            for (int col = 0; col < 4; ++col)
                pos[(size_t) square[r][col]] = { -h + cell * ((float) col + 0.5f), -h + cell * ((float) r + 0.5f) };
        for (int n = 1; n <= 16; ++n)
            if (n == 1) s.moveTo (pos[1]); else s.lineTo (pos[(size_t) n]);
        s.circle (pos[1], 0.07f, 12);
    }

    void ouroboros (Sketch& s, const GlyphCtx& c)
    {
        const float gap = 0.32f;
        s.arc (O, 0.82f, 0.82f, gap, twoPi - 0.08f, 64);
        s.arc (O, 0.62f, 0.62f, gap, twoPi - 0.08f, 56);
        s.ellipse (Sketch::polar (O, 0.72f, gap * 0.45f), 0.16f, 0.12f, 20);
        s.circle (Sketch::polar (O, 0.76f, gap * 0.5f), 0.025f + 0.02f * c.kick, 8);
        for (int i = 0; i < 18; ++i)
        {
            const float a = gap + 0.15f + (twoPi - gap - 0.3f) * (float) i / 18.0f;
            s.line (Sketch::polar (O, 0.62f, a), Sketch::polar (O, 0.82f, a + 0.08f));
        }
    }

    // ================================================================== SACRED GEOMETRY
    void seedOfLife (Sketch& s, const GlyphCtx&)
    {
        const float r = 0.44f;
        s.circle (O, r, 36);
        for (int i = 0; i < 6; ++i)
            s.circle (Sketch::polar (O, r, twoPi * (float) i / 6.0f), r, 36);
        s.circle (O, r * 2.0f + 0.04f, 64);
    }

    void flowerOfLife (Sketch& s, const GlyphCtx&)
    {
        const float r = 0.25f;
        s.circle (O, r, 28);
        for (int ring = 1; ring <= 2; ++ring)
            for (int k = 0; k < 6; ++k)
                for (int j = 0; j < ring; ++j)
                {
                    const P a = Sketch::polar (O, r * (float) ring, twoPi * (float) k / 6.0f);
                    const P b = Sketch::polar (O, r * (float) ring, twoPi * (float) (k + 1) / 6.0f);
                    s.circle (a + (b - a) * ((float) j / (float) ring), r, 28);
                }
        s.circle (O, r * 3.0f + 0.03f, 72);
    }

    void metatron (Sketch& s, const GlyphCtx&)
    {
        std::array<P, 13> c {};
        c[0] = O;
        for (int i = 0; i < 6; ++i)
        {
            c[(size_t) (1 + i)] = Sketch::polar (O, 0.42f, twoPi * (float) i / 6.0f);
            c[(size_t) (7 + i)] = Sketch::polar (O, 0.84f, twoPi * (float) i / 6.0f);
        }
        for (const auto& p : c)
            s.circle (p, 0.14f, 18);
        for (int i = 0; i < 13; ++i)
            for (int j = i + 1; j < 13; ++j)
                s.line (c[(size_t) i], c[(size_t) j]);
    }

    void vesica (Sketch& s, const GlyphCtx& c)
    {
        s.circle ({ -0.32f, 0.0f }, 0.64f, 56);
        s.circle ({ 0.32f, 0.0f }, 0.64f, 56);
        s.line (0.0f, -0.95f, 0.0f, 0.95f);
        s.eye (O, 0.22f, 0.12f * (1.0f - 0.5f * c.kick));
    }

    void triquetra (Sketch& s, const GlyphCtx&)
    {
        for (int k = 0; k < 3; ++k)
            lens (s, O, Sketch::polar (O, 0.95f, twoPi * (float) k / 3.0f), 0.32f, 20);
        s.circle (O, 0.5f, 48);
    }

    void borromean (Sketch& s, const GlyphCtx& c)
    {
        for (int k = 0; k < 3; ++k)
            s.circle (Sketch::polar (O, 0.33f + 0.05f * c.bass, twoPi * (float) k / 3.0f + c.t * 0.1f), 0.55f, 48);
    }

    void goldenSpiral (Sketch& s, const GlyphCtx& c)
    {
        const float phi = 1.618034f;
        const int n = 160;
        for (int i = 0; i <= n; ++i)
        {
            const float th = 4.6f * pi * (float) i / (float) n;
            const float r = 0.035f * std::pow (phi, th / (pi * 0.5f));
            const P p = Sketch::polar (O, juce::jmin (1.05f, r), th + c.t * 0.15f);
            if (i == 0) s.moveTo (p); else s.lineTo (p);
        }
        s.circle (O, 1.0f, 64);
    }

    void sunWheel (Sketch& s, const GlyphCtx& c)
    {
        s.circle (O, 0.55f, 48);
        s.circle (O, 0.45f, 40);
        s.star (O, 0.45f, 8, 3, c.t * 0.1f);
        for (int i = 0; i < 16; ++i)
        {
            const float a = twoPi * (float) i / 16.0f;
            s.line (Sketch::polar (O, 0.62f, a), Sketch::polar (O, (i % 2 == 0 ? 0.98f : 0.8f) * (0.9f + 0.1f * c.kick), a));
        }
    }

    void crescentStar (Sketch& s, const GlyphCtx&)
    {
        s.arc ({ 0.0f, 0.1f }, 0.75f, 0.75f, pi * 0.62f, pi * 1.38f, 40);
        s.arc ({ 0.0f, -0.12f }, 0.6f, 0.6f, pi * 0.62f, pi * 1.38f, 36);
        s.line (Sketch::polar ({ 0.0f, 0.1f }, 0.75f, pi * 0.62f), Sketch::polar ({ 0.0f, -0.12f }, 0.6f, pi * 0.62f));
        s.line (Sketch::polar ({ 0.0f, 0.1f }, 0.75f, pi * 1.38f), Sketch::polar ({ 0.0f, -0.12f }, 0.6f, pi * 1.38f));
        s.star ({ 0.0f, -0.45f }, 0.28f, 5, 2, 0.0f);
    }

    void yantra (Sketch& s, const GlyphCtx&)
    {
        s.polygon (O, 1.0f, 4, pi * 0.25f);
        s.circle (O, 0.66f, 56);
        for (int i = 0; i < 12; ++i)
            lens (s, Sketch::polar (O, 0.66f, twoPi * (float) i / 12.0f), Sketch::polar (O, 0.86f, twoPi * (float) i / 12.0f), 0.05f, 8);
        const float up[] = { 0.58f, 0.42f, 0.26f };
        const float dn[] = { 0.54f, 0.38f, 0.22f, 0.1f };
        for (float r : up) s.polygon ({ 0.0f, 0.04f }, r, 3, 0.0f);
        for (float r : dn) s.polygon ({ 0.0f, -0.04f }, r, 3, pi);
    }

    void lotus (Sketch& s, const GlyphCtx& c)
    {
        const float open = 0.75f + 0.25f * c.volume;
        for (int i = 0; i < 8; ++i)
        {
            const float a = twoPi * (float) i / 8.0f;
            lens (s, Sketch::polar (O, 0.2f, a), Sketch::polar (O, 0.95f * open, a), 0.13f, 14);
            lens (s, Sketch::polar (O, 0.12f, a + pi / 8.0f), Sketch::polar (O, 0.6f * open, a + pi / 8.0f), 0.1f, 10);
        }
        s.circle (O, 0.16f, 20);
    }

    void labyrinth (Sketch& s, const GlyphCtx& c)
    {
        for (int ring = 0; ring < 6; ++ring)
        {
            const float r = 0.2f + 0.15f * (float) ring;
            const float gapAt = twoPi * rnd (c.seed, ring);
            s.arc (O, r, r, gapAt + 0.25f, gapAt + twoPi - 0.25f, 24 + ring * 8);
            const float link = gapAt + pi * (0.6f + 0.6f * rnd (c.seed, ring + 20));
            if (ring < 5)
                s.line (Sketch::polar (O, r, link), Sketch::polar (O, r + 0.15f, link));
        }
    }

    void spirograph (Sketch& s, const GlyphCtx& c)
    {
        const float sets[3][3] = { { 5.0f, 3.0f, 5.0f }, { 7.0f, 4.0f, 3.0f }, { 8.0f, 3.0f, 6.0f } };
        const auto& k = sets[c.seed % 3];
        const float R = k[0], r = k[1], d = k[2];
        const float scale = 1.0f / (R - r + d);
        const int n = 420;
        for (int i = 0; i <= n; ++i)
        {
            const float t = twoPi * r * (float) i / (float) n;
            const P p (((R - r) * std::cos (t) + d * std::cos ((R - r) / r * t)) * scale,
                       ((R - r) * std::sin (t) - d * std::sin ((R - r) / r * t)) * scale);
            if (i == 0) s.moveTo (p); else s.lineTo (p);
        }
    }

    void lissajous (Sketch& s, const GlyphCtx& c)
    {
        const int n = 300;
        const float ph = c.t * 0.25f;
        for (int i = 0; i <= n; ++i)
        {
            const float t = twoPi * (float) i / (float) n;
            const P p (0.95f * std::sin (3.0f * t + ph), 0.95f * std::sin (4.0f * t));
            if (i == 0) s.moveTo (p); else s.lineTo (p);
        }
    }

    void pyramid (Sketch& s, const GlyphCtx& c)
    {
        s.moveTo (-0.9f, 0.8f); s.lineTo (0.0f, -0.35f); s.lineTo (0.9f, 0.8f); s.close();
        for (int i = 1; i < 4; ++i)
        {
            const float y = -0.35f + 1.15f * (float) i / 4.0f;
            const float hw = 0.9f * (y + 0.35f) / 1.15f;
            s.line (-hw, y, hw, y);
        }
        const float lift = 0.1f + 0.08f * c.kick;
        s.polygon ({ 0.0f, -0.62f - lift }, 0.26f, 3, 0.0f);
        for (int i = 0; i < 9; ++i)
        {
            const float a = -pi * 0.5f + pi * (float) i / 8.0f;
            s.line (Sketch::polar ({ 0.0f, -0.62f - lift }, 0.32f, a), Sketch::polar ({ 0.0f, -0.62f - lift }, 0.5f + 0.15f * c.high, a));
        }
    }

    // ================================================================== STRANGE
    void watcher (Sketch& s, const GlyphCtx& c)
    {
        const float open = juce::jmax (0.08f, 1.0f - 0.85f * c.kick);
        s.eye (O, 0.95f, 0.5f * open, 32);
        const float ir = 0.32f * juce::jmin (1.0f, open * 1.6f);
        s.circle (O, ir, 32);
        s.circle (O, ir * 0.38f * (1.0f + 0.4f * c.bass), 16);
        for (int i = 0; i <= 60; ++i)
        {
            const float th = 3.0f * twoPi * (float) i / 60.0f;
            const P p = Sketch::polar (O, ir * 0.9f * (float) i / 60.0f, th - c.t * 0.8f);
            if (i == 0) s.moveTo (p); else s.lineTo (p);
        }
    }

    void threeEyedFace (Sketch& s, const GlyphCtx& c)
    {
        s.ellipse (O, 0.66f, 0.92f, 56);
        const float open = 1.0f - 0.7f * c.kick;
        s.eye ({ -0.27f, -0.05f }, 0.17f, 0.08f * open, 14);
        s.eye ({ 0.27f, -0.05f }, 0.17f, 0.08f * open, 14);
        s.eye ({ 0.0f, -0.45f }, 0.13f, 0.1f * (1.0f - open * 0.5f), 12);
        s.circle ({ -0.27f, -0.05f }, 0.04f, 8);
        s.circle ({ 0.27f, -0.05f }, 0.04f, 8);
        s.line (-0.24f, 0.45f, 0.24f, 0.45f);
    }

    void hourglassOfTime (Sketch& s, const GlyphCtx& c)
    {
        s.line (-0.6f, -0.92f, 0.6f, -0.92f);
        s.line (-0.6f, 0.92f, 0.6f, 0.92f);
        s.moveTo (-0.5f, -0.88f); s.lineTo (0.0f, 0.0f); s.lineTo (-0.5f, 0.88f);
        s.moveTo (0.5f, -0.88f);  s.lineTo (0.0f, 0.0f); s.lineTo (0.5f, 0.88f);
        const float sand = std::fmod (c.t * 0.05f, 1.0f);
        s.line (-0.45f * (1.0f - sand), -0.8f * (1.0f - sand), 0.45f * (1.0f - sand), -0.8f * (1.0f - sand));
        s.line (-0.45f * sand, 0.85f - 0.75f * sand, 0.45f * sand, 0.85f - 0.75f * sand);
        s.line (0.0f, 0.0f, 0.0f, 0.85f);
    }

    // ================================================================== INTERDIMENSIONAL
    void tesseract (Sketch& s, const GlyphCtx& c)
    {
        std::array<P, 16> p {};
        for (int i = 0; i < 16; ++i)
            p[(size_t) i] = project4 ({ (i & 1) ? 1.0f : -1.0f, (i & 2) ? 1.0f : -1.0f,
                                        (i & 4) ? 1.0f : -1.0f, (i & 8) ? 1.0f : -1.0f }, c);
        for (int i = 0; i < 16; ++i)
            for (int bit = 0; bit < 4; ++bit)
            {
                const int j = i ^ (1 << bit);
                if (j > i)
                    s.line (p[(size_t) i], p[(size_t) j]);
            }
    }

    void sixteenCell (Sketch& s, const GlyphCtx& c)
    {
        std::array<P, 8> p {};
        for (int i = 0; i < 8; ++i)
        {
            const float v = (i % 2 == 0) ? 1.35f : -1.35f;
            const int axis = i / 2;
            p[(size_t) i] = project4 ({ axis == 0 ? v : 0.0f, axis == 1 ? v : 0.0f, axis == 2 ? v : 0.0f, axis == 3 ? v : 0.0f }, c);
        }
        for (int i = 0; i < 8; ++i)
            for (int j = i + 1; j < 8; ++j)
                if (i / 2 != j / 2)
                    s.line (p[(size_t) i], p[(size_t) j]);
    }

    void hypersphere (Sketch& s, const GlyphCtx& c)
    {
        // Hopf fibres: great circles of the 3-sphere, each a perfect circle in 4D
        for (int f = 0; f < 9; ++f)
        {
            const float eta = 0.25f + 1.1f * (float) (f % 3) / 2.0f * 0.5f + 0.15f * (float) (f / 3);
            const float p1 = twoPi * (float) f / 9.0f, p2 = p1 * 2.0f;
            const float r1 = std::cos (eta) * 1.25f, r2 = std::sin (eta) * 1.25f;
            for (int i = 0; i <= 40; ++i)
            {
                const float th = twoPi * (float) i / 40.0f;
                const P q = project4 ({ r1 * std::cos (th + p1), r1 * std::sin (th + p1), r2 * std::cos (th + p2), r2 * std::sin (th + p2) }, c);
                if (i == 0) s.moveTo (q); else s.lineTo (q);
            }
        }
    }

    void merkaba (Sketch& s, const GlyphCtx& c)
    {
        const V3 tet[2][4] = { { { 1, 1, 1 }, { -1, -1, 1 }, { -1, 1, -1 }, { 1, -1, -1 } },
                               { { -1, -1, -1 }, { 1, 1, -1 }, { 1, -1, 1 }, { -1, 1, 1 } } };
        for (int t = 0; t < 2; ++t)
        {
            std::array<P, 4> p {};
            const float dir = t == 0 ? 1.0f : -1.0f;
            for (int i = 0; i < 4; ++i)
            {
                GlyphCtx cc = c;
                cc.spin = c.spin * dir;
                p[(size_t) i] = project (turn3 (tet[t][i], cc));
            }
            for (int i = 0; i < 4; ++i)
                for (int j = i + 1; j < 4; ++j)
                    s.line (p[(size_t) i], p[(size_t) j]);
        }
    }

    void icosahedron (Sketch& s, const GlyphCtx& c)
    {
        const float g = 1.618034f, k = 1.0f / std::sqrt (1.0f + g * g) * 1.3f;
        const V3 v[12] = { { 0, 1, g }, { 0, -1, g }, { 0, 1, -g }, { 0, -1, -g }, { 1, g, 0 }, { -1, g, 0 },
                           { 1, -g, 0 }, { -1, -g, 0 }, { g, 0, 1 }, { -g, 0, 1 }, { g, 0, -1 }, { -g, 0, -1 } };
        std::array<P, 12> p {};
        for (int i = 0; i < 12; ++i)
            p[(size_t) i] = project (turn3 ({ v[i].x * k, v[i].y * k, v[i].z * k }, c));
        for (int i = 0; i < 12; ++i)
            for (int j = i + 1; j < 12; ++j)
            {
                const float dx = v[i].x - v[j].x, dy = v[i].y - v[j].y, dz = v[i].z - v[j].z;
                if (std::abs (dx * dx + dy * dy + dz * dz - 4.0f) < 0.01f)
                    s.line (p[(size_t) i], p[(size_t) j]);
            }
    }

    void torusKnot (Sketch& s, const GlyphCtx& c)
    {
        const int n = 260;
        for (int i = 0; i <= n; ++i)
        {
            const float t = twoPi * (float) i / (float) n;
            const V3 v { (std::sin (t) + 2.0f * std::sin (2.0f * t)) / 2.4f,
                         (std::cos (t) - 2.0f * std::cos (2.0f * t)) / 2.4f,
                         -std::sin (3.0f * t) / 2.4f };
            const P p = project (turn3 (v, c));
            if (i == 0) s.moveTo (p); else s.lineTo (p);
        }
    }

    void torusField (Sketch& s, const GlyphCtx& c)
    {
        for (int ring = 0; ring < 14; ++ring)
        {
            const float a = twoPi * (float) ring / 14.0f;
            for (int i = 0; i <= 28; ++i)
            {
                const float b = twoPi * (float) i / 28.0f;
                const float R = 1.0f + 0.45f * std::cos (b);
                const V3 v { R * std::cos (a) * 0.85f, 0.45f * std::sin (b) * 0.85f, R * std::sin (a) * 0.85f };
                const P p = project (turn3 (v, c));
                if (i == 0) s.moveTo (p); else s.lineTo (p);
            }
        }
    }

    void portal (Sketch& s, const GlyphCtx& c)
    {
        for (int i = 0; i < 9; ++i)
        {
            const float z = std::fmod ((float) i / 9.0f + c.t * 0.12f, 1.0f);
            const float r = 0.05f + z * z * 1.0f;
            const int sides = 6 + (i % 3) * 2;
            s.polygon (O, r, sides, c.spin * (i % 2 == 0 ? 1.0f : -1.0f) * 0.4f + (float) i);
        }
        for (int k = 0; k < 6; ++k)
            s.line (Sketch::polar (O, 0.06f, twoPi * (float) k / 6.0f), Sketch::polar (O, 1.05f, twoPi * (float) k / 6.0f));
    }

    void corridor (Sketch& s, const GlyphCtx& c)
    {
        for (int i = 0; i < 8; ++i)
        {
            const float z = std::fmod ((float) i / 8.0f + c.t * 0.1f, 1.0f);
            const float h = 0.04f + z * z * 1.0f;
            s.moveTo (-h, -h * 0.75f); s.lineTo (h, -h * 0.75f); s.lineTo (h, h * 0.75f); s.lineTo (-h, h * 0.75f); s.close();
        }
        const P corners[4] = { { -1.0f, -0.75f }, { 1.0f, -0.75f }, { 1.0f, 0.75f }, { -1.0f, 0.75f } };
        for (const auto& k : corners)
            s.line (k * 0.04f, k);
        // a doorway at the far end
        s.moveTo (-0.03f, 0.03f); s.lineTo (-0.03f, -0.02f); s.lineTo (0.03f, -0.02f); s.lineTo (0.03f, 0.03f);
    }

    void dimensionalRift (Sketch& s, const GlyphCtx& c)
    {
        // a jagged tear with geometry leaking through
        const int n = 14;
        for (int side = 0; side < 2; ++side)
        {
            const float sg = side == 0 ? -1.0f : 1.0f;
            for (int i = 0; i <= n; ++i)
            {
                const float y = -1.0f + 2.0f * (float) i / (float) n;
                const float open = std::sin (pi * (float) i / (float) n) * (0.18f + 0.2f * c.volume);
                const float jag = (rnd (c.seed, i) - 0.5f) * 0.12f;
                const P p (sg * open + jag, y);
                if (i == 0) s.moveTo (p); else s.lineTo (p);
            }
        }
        for (int k = 0; k < 4; ++k)
            s.polygon ({ 0.0f, -0.5f + 0.33f * (float) k }, 0.08f + 0.06f * c.kick, 3 + k, c.t * (k % 2 == 0 ? 1.0f : -1.0f));
    }

    void orrery (Sketch& s, const GlyphCtx& c)
    {
        s.circle (O, 0.12f + 0.04f * c.kick, 20);
        for (int i = 0; i < 4; ++i)
        {
            const float r = 0.3f + 0.2f * (float) i;
            s.ellipse (O, r, r * 0.42f, 48);
            const float a = c.t * (0.6f / (1.0f + (float) i)) + (float) i * 1.7f;
            s.circle ({ std::sin (a) * r, -std::cos (a) * r * 0.42f }, 0.04f + 0.015f * (float) i, 12);
        }
    }
}

const std::vector<Glyph>& glyphLibrary()
{
    static const std::vector<Glyph> lib {
        // occult
        { "PENTAGRAM",            pentagram,       Realm::Occult,    5.0f },
        { "HEXAGRAM",             hexagram,        Realm::Occult,    6.0f },
        { "HEPTAGRAM",            heptagram,       Realm::Occult,    7.0f },
        { "OCTAGRAM",             octagram,        Realm::Occult,    8.0f },
        { "ENNEAGRAM",            enneagram,       Realm::Occult,    9.0f },
        { "EYE IN THE TRIANGLE",  eyeInTriangle,   Realm::Occult,    6.0f },
        { "CHAOS STAR",           chaosStar,       Realm::Occult,    8.0f },
        { "THE FOUR ELEMENTS",    elements,        Realm::Occult,    4.0f },
        { "SULFUR",               sulfur,          Realm::Occult,    3.0f },
        { "MERCURY",              mercury,         Realm::Occult,    3.0f },
        { "ANKH",                 ankh,            Realm::Occult,    3.0f },
        { "SIGIL",                sigil,           Realm::Occult,    5.0f },
        { "RUNE STAVE",           runeStave,       Realm::Occult,    4.0f },
        { "MAGIC SQUARE",         magicSquare,     Realm::Occult,    8.0f },
        { "OUROBOROS",            ouroboros,       Realm::Occult,    7.0f },
        // sacred geometry
        { "SEED OF LIFE",         seedOfLife,      Realm::Sacred,    7.0f },
        { "FLOWER OF LIFE",       flowerOfLife,    Realm::Sacred,   12.0f },
        { "METATRON'S CUBE",      metatron,        Realm::Sacred,   10.0f },
        { "VESICA PISCIS",        vesica,          Realm::Sacred,    4.0f },
        { "TRIQUETRA",            triquetra,       Realm::Sacred,    6.0f },
        { "BORROMEAN RINGS",      borromean,       Realm::Sacred,    6.0f },
        { "GOLDEN SPIRAL",        goldenSpiral,    Realm::Sacred,    5.0f },
        { "SUN WHEEL",            sunWheel,        Realm::Sacred,    8.0f },
        { "CRESCENT AND STAR",    crescentStar,    Realm::Sacred,    4.0f },
        { "YANTRA",               yantra,          Realm::Sacred,   10.0f },
        { "LOTUS",                lotus,           Realm::Sacred,    8.0f },
        { "LABYRINTH",            labyrinth,       Realm::Sacred,    8.0f },
        { "SPIROGRAPH",           spirograph,      Realm::Sacred,    9.0f },
        { "LISSAJOUS KNOT",       lissajous,       Realm::Sacred,    6.0f },
        { "PYRAMID OF LIGHT",     pyramid,         Realm::Sacred,    5.0f },
        // strange
        { "THE WATCHER",          watcher,         Realm::Strange,   5.0f },
        { "THREE-EYED FACE",      threeEyedFace,   Realm::Strange,   5.0f },
        { "HOURGLASS",            hourglassOfTime, Realm::Strange,   4.0f },
        { "ORRERY",               orrery,          Realm::Strange,   6.0f },
        // interdimensional
        { "TESSERACT",            tesseract,       Realm::Dimension, 8.0f },
        { "16-CELL",              sixteenCell,     Realm::Dimension, 8.0f },
        { "HYPERSPHERE",          hypersphere,     Realm::Dimension, 9.0f },
        { "MERKABA",              merkaba,         Realm::Dimension, 6.0f },
        { "ICOSAHEDRON",          icosahedron,     Realm::Dimension, 8.0f },
        { "TORUS KNOT",           torusKnot,       Realm::Dimension, 6.0f },
        { "TORUS FIELD",          torusField,      Realm::Dimension, 10.0f },
        { "PORTAL",               portal,          Realm::Dimension, 8.0f },
        { "INFINITE CORRIDOR",    corridor,        Realm::Dimension, 6.0f },
        { "RIFT",                 dimensionalRift, Realm::Dimension, 5.0f },
    };
    return lib;
}
}

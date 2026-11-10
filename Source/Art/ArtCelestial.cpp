// =================================================================================================
//  CELESTIAL  (constellations use their real star patterns)
// =================================================================================================
#include "ArtKit.h"
#include <cmath>
#include <utility>

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

void crescent (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.S() * 0.3f;
    K::halo (g, c, r * 3.0f, k.haze.withAlpha (0.2f + 0.1f * f.mid));
    juce::Path moonShape;
    moonShape.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    moonShape.addEllipse (c.x - r * 0.55f, c.y - r * 1.15f, r * 2.0f, r * 2.0f);
    moonShape.setUsingNonZeroWinding (false);
    {
        juce::Graphics::ScopedSaveState state (g);
        juce::Path disk;
        disk.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.reduceClipRegion (disk);
        g.setColour (k.line.withAlpha (0.9f));
        g.fillPath (moonShape);
    }
    K::stars (g, f, f.seed, 10, 1.0f, k.line, f.hat);
}

void fullMoon (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.S() * 0.36f;
    K::moon (g, c, r, k.line.withAlpha (k.light ? 0.25f : 0.85f), 1.0f + f.bass);
    juce::Path craters;
    for (int i = 0; i < 7; ++i)
    {
        const float a = K::hash (f.seed, i) * twoPi, d = K::hash (f.seed, i + 10) * r * 0.7f;
        const float cr = r * (0.08f + 0.12f * K::hash (f.seed, i + 20));
        craters.addEllipse (c.x + std::sin (a) * d - cr, c.y - std::cos (a) * d - cr, cr * 2.0f, cr * 2.0f);
    }
    g.setColour (k.light ? k.sil.withAlpha (0.35f) : k.bg.withAlpha (0.25f));
    g.strokePath (craters, K::pen (1.0f));
    const float drift = (f.life - 0.5f) * f.W() * 0.9f;
    K::cloud (g, c.x + drift, c.y + r * 0.4f, r * 2.2f, r * 0.5f, k.sil.withAlpha (0.85f), k.haze.withAlpha (0.1f), f.seed);
}

void moonPhases (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const int n = 8;
    const float r = juce::jmin (f.H() * 0.38f, f.W() / (float) n * 0.36f);
    for (int i = 0; i < n; ++i)
    {
        const Pt c (f.box.getX() + f.W() * ((float) i + 0.5f) / (float) n, f.box.getCentreY());
        const float lit = 0.4f + 0.6f * juce::jmax (0.0f, std::sin (f.t * 1.2f - (float) i * 0.5f));
        g.setColour (k.line.withAlpha (0.3f));
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 0.7f);
        const float phase = (float) i / (float) n;
        juce::Path lightPart;
        lightPart.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (lightPart);
        const float off = (phase * 2.0f - 1.0f) * r * 2.0f;
        g.setColour (k.line.withAlpha (0.75f * lit));
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour (k.bg);
        g.fillEllipse (c.x - r + off, c.y - r, r * 2.0f, r * 2.0f);
    }
}

void ringedPlanet (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.H() * 0.3f;
    const float tilt = -0.35f + 0.05f * std::sin (f.t * 0.2f);
    juce::Path back, front;
    back.addCentredArc (c.x, c.y, r * 2.2f, r * 0.55f, tilt, -pi * 0.5f, pi * 0.5f, true);
    front.addCentredArc (c.x, c.y, r * 2.2f, r * 0.55f, tilt, pi * 0.5f, pi * 1.5f, true);
    K::glowLine (g, back, k.line.withAlpha (0.4f), k.glow, 0.8f, 0.2f, k.light);
    juce::Path planet;
    planet.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    g.setColour (k.sil);
    g.fillPath (planet);
    juce::ColourGradient shade (k.line.withAlpha (k.light ? 0.15f : 0.35f), c.x - r * 0.6f, c.y - r * 0.6f, k.line.withAlpha (0.0f), c.x + r * 0.6f, c.y + r * 0.6f, false);
    g.setGradientFill (shade);
    g.fillPath (planet);
    K::glowLine (g, planet, k.line.withAlpha (0.6f), k.glow, 0.8f, 0.4f + 0.4f * f.bass, k.light);
    K::glowLine (g, front, k.line.withAlpha (0.85f), k.glow, 1.0f, 0.4f + 0.4f * f.bass, k.light);
    for (int i = 1; i <= 2; ++i)
    {
        juce::Path ring;
        ring.addCentredArc (c.x, c.y, r * (2.2f + 0.18f * (float) i), r * (0.55f + 0.05f * (float) i), tilt, pi * 0.5f, pi * 1.5f, true);
        g.setColour (k.line.withAlpha (0.25f));
        g.strokePath (ring, K::pen (0.6f));
    }
}

void orrery (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    K::halo (g, c, f.S() * 0.08f, k.warm.withAlpha (0.9f));
    for (int i = 1; i <= 5; ++i)
    {
        const float r = f.S() * 0.09f * (float) i;
        g.setColour (k.line.withAlpha (0.2f));
        g.drawEllipse (c.x - r, c.y - r * 0.5f, r * 2.0f, r, 0.6f);
        const float a = f.t * (0.8f + 0.6f * f.mid) / (float) i + K::hash (f.seed, i) * twoPi;
        const Pt pl (c.x + std::cos (a) * r, c.y + std::sin (a) * r * 0.5f);
        g.setColour (k.line.withAlpha (0.9f));
        g.fillEllipse (pl.x - 2.5f, pl.y - 2.5f, 5.0f, 5.0f);
        g.setColour (k.line.withAlpha (0.15f));
        g.drawLine (c.x, c.y, pl.x, pl.y, 0.5f);
    }
}

void eclipse (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.S() * 0.28f;
    K::halo (g, c, r * 2.4f, k.line.withAlpha (0.25f + 0.2f * f.bass));
    g.setColour (k.line.withAlpha (0.35f));
    for (int i = 0; i < 48; ++i)
    {
        const float a = (float) i / 48.0f * twoPi + f.t * 0.02f;
        const float len = r * (1.15f + 0.4f * K::noise (f.seed + (juce::uint32) i, f.t * 0.5f) + 0.3f * f.kick);
        g.drawLine (c.x + std::sin (a) * r * 1.02f, c.y - std::cos (a) * r * 1.02f, c.x + std::sin (a) * len, c.y - std::cos (a) * len, 0.6f);
    }
    g.setColour (k.light ? k.sil : k.bg);
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    g.setColour (k.line.withAlpha (0.8f));
    g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.0f);
}

void comet (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const float x = f.X (-0.1f + 1.2f * f.life);
    const float y = f.Y (0.6f - 0.3f * f.life);
    const float tailDir = f.flip ? 1.0f : -1.0f;
    for (int i = 0; i < 3; ++i)
    {
        juce::Path tail;
        tail.startNewSubPath (x, y);
        tail.quadraticTo (x + tailDir * f.H() * 1.2f, y + f.H() * (0.1f + 0.06f * (float) i), x + tailDir * f.H() * (2.2f + 0.4f * (float) i), y + f.H() * (0.25f + 0.1f * (float) i));
        g.setColour (k.line.withAlpha (0.25f - 0.06f * (float) i));
        g.strokePath (tail, K::pen (1.0f + (float) (2 - i)));
    }
    K::halo (g, { x, y }, f.H() * 0.25f, k.glow.withAlpha (0.4f + 0.3f * f.high));
    g.setColour (k.line);
    g.fillEllipse (x - 2.5f, y - 2.5f, 5.0f, 5.0f);
}

void meteorShower (G& g, const Frame& f)
{
    const auto& k = f.ink;
    K::stars (g, f, f.seed, 30, 1.0f, k.line, f.hat);
    for (int i = 0; i < 6; ++i)
    {
        const float period = 2.0f + 2.0f * K::hash (f.seed, i);
        const float ph = std::fmod (f.t + K::hash (f.seed, i + 9) * period, period) / period;
        if (ph > 0.3f)
            continue;
        const float tt = ph / 0.3f;
        const float sx = K::hash (f.seed, i + 20), sy = 0.1f + 0.4f * K::hash (f.seed, i + 30);
        const Pt head (f.X (sx + 0.25f * tt), f.Y (sy + 0.25f * tt));
        const Pt tailEnd (head.x - f.DX (0.08f), head.y - f.H() * 0.08f);
        juce::ColourGradient grad (k.line.withAlpha (1.0f - tt), head.x, head.y, k.line.withAlpha (0.0f), tailEnd.x, tailEnd.y, false);
        g.setGradientFill (grad);
        g.drawLine ({ tailEnd, head }, 1.2f);
    }
}

void constellation (G& g, const Frame& f, const std::vector<Pt>& s, const std::vector<std::pair<int, int>>& links)
{
    const auto& k = f.ink;
    K::stars (g, f, f.seed, 26, 1.0f, k.line.withAlpha (0.6f), f.hat);
    const float reveal = K::ease (f.t / 5.0f);
    juce::Path p;
    for (size_t i = 0; i < links.size(); ++i)
    {
        if ((float) i / (float) links.size() > reveal)
            break;
        const auto a = s[(size_t) links[i].first], b = s[(size_t) links[i].second];
        p.startNewSubPath (f.P (a.x, a.y));
        p.lineTo (f.P (b.x, b.y));
    }
    K::glowLine (g, p, k.line.withAlpha (0.6f), k.glow, 0.7f, 0.4f + 0.3f * f.mid, k.light);
    for (const auto& st : s)
        K::halo (g, f.P (st.x, st.y), f.S() * (0.035f + 0.02f * f.hat), k.line.withAlpha (0.9f));
}

void orion (G& g, const Frame& f)
{
    constellation (g, f, { { 0.30f, 0.20f }, { 0.62f, 0.25f }, { 0.48f, 0.10f }, { 0.42f, 0.50f }, { 0.48f, 0.48f }, { 0.54f, 0.46f }, { 0.36f, 0.82f }, { 0.66f, 0.80f } },
                   { { 0, 2 }, { 2, 1 }, { 0, 3 }, { 1, 5 }, { 3, 4 }, { 4, 5 }, { 3, 6 }, { 5, 7 } });
}

void plough (G& g, const Frame& f)
{
    constellation (g, f, { { 0.1f, 0.35f }, { 0.27f, 0.3f }, { 0.4f, 0.37f }, { 0.52f, 0.45f }, { 0.55f, 0.68f }, { 0.8f, 0.72f }, { 0.84f, 0.47f } },
                   { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 4 }, { 4, 5 }, { 5, 6 }, { 6, 3 } });
}

void cassiopeia (G& g, const Frame& f)
{
    constellation (g, f, { { 0.1f, 0.38f }, { 0.3f, 0.65f }, { 0.48f, 0.42f }, { 0.66f, 0.66f }, { 0.9f, 0.35f } },
                   { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 4 } });
}

void cygnus (G& g, const Frame& f)
{
    // the Northern Cross
    constellation (g, f, { { 0.5f, 0.1f }, { 0.5f, 0.35f }, { 0.5f, 0.55f }, { 0.5f, 0.92f }, { 0.2f, 0.45f }, { 0.8f, 0.6f } },
                   { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 4, 1 }, { 1, 5 } });
}

void galaxy (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    K::blob (g, c, f.W() * 0.18f, f.H() * 0.12f, k.glow.withAlpha (0.35f + 0.2f * f.bass));
    const float rot = f.t * (0.05f + 0.1f * f.mid);
    for (int i = 0; i < 260; ++i)
    {
        const float arm = (float) (i % 2) * pi;
        const float d = K::hash (f.seed, i);
        const float a = arm + d * 3.5f + rot + (K::hash (f.seed, i + 300) - 0.5f) * 0.5f;
        const float r = d * 0.45f;
        const float x = c.x + std::cos (a) * r * f.W();
        const float y = c.y + std::sin (a) * r * f.H() * 0.55f;
        g.setColour (k.line.withAlpha (0.25f + 0.6f * (1.0f - d) * K::hash (f.seed, i + 600)));
        g.fillRect (x, y, 1.3f, 1.3f);
    }
}

void nebula (G& g, const Frame& f)
{
    const auto& k = f.ink;
    for (int i = 0; i < 7; ++i)
    {
        const float u = 0.2f + 0.6f * K::hash (f.seed, i) + 0.03f * std::sin (f.t * 0.1f + (float) i);
        const float v = 0.25f + 0.5f * K::hash (f.seed, i + 10);
        const auto col = (i % 3 == 0 ? k.accent : (i % 3 == 1 ? k.glow : k.haze)).withAlpha (0.14f + 0.08f * f.flow);
        K::blob (g, f.P (u, v), f.W() * (0.12f + 0.15f * K::hash (f.seed, i + 20)), f.H() * (0.1f + 0.15f * K::hash (f.seed, i + 30)), col);
    }
    K::stars (g, f, f.seed + 5u, 60, 1.0f, k.line, f.hat);
}

void astrolabe (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.S() * 0.44f;
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
    a.addCentredArc (c.x, c.y + r * 0.2f, r * 0.55f, r * 0.55f, f.t * 0.08f, 0.0f, twoPi, true);
    for (int i = 0; i < 3; ++i)
        a.addCentredArc (c.x, c.y + r * 0.6f, r * (0.5f + 0.15f * (float) i), r * (0.5f + 0.15f * (float) i), 0.0f, -1.2f, 1.2f, true);
    K::glowLine (g, a, k.line.withAlpha (0.8f), k.glow, 0.8f, 0.3f + 0.4f * f.mid, k.light);

    const float rule = f.t * (0.12f + 0.2f * f.energy);
    juce::Path alidade;
    alidade.startNewSubPath (c.x - std::sin (rule) * r, c.y + std::cos (rule) * r);
    alidade.lineTo (c.x + std::sin (rule) * r, c.y - std::cos (rule) * r);
    K::glowLine (g, alidade, k.accent, k.glow, 1.3f, 0.6f, k.light);
}

void starChart (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float R = f.S() * 0.42f;
    for (int i = 1; i <= 3; ++i)
    {
        const float r = R * (float) i / 3.0f;
        g.setColour (k.line.withAlpha (0.25f));
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 0.6f);
    }
    for (int i = 0; i < 12; ++i)
    {
        const float a = (float) i / 12.0f * twoPi;
        g.drawLine (c.x, c.y, c.x + std::sin (a) * R, c.y - std::cos (a) * R, 0.4f);
    }
    juce::Path ecl;
    ecl.addCentredArc (c.x, c.y + R * 0.2f, R * 0.8f, R * 0.55f, 0.3f + f.t * 0.02f, 0.0f, twoPi, true);
    K::glowLine (g, ecl, k.accent, k.glow, 0.9f, 0.5f, k.light);
    for (int i = 0; i < 40; ++i)
    {
        const float a = K::hash (f.seed, i) * twoPi, d = std::sqrt (K::hash (f.seed, i + 40)) * R;
        const float s = 1.0f + 2.0f * K::hash (f.seed, i + 80);
        g.setColour (k.line.withAlpha (0.5f + 0.5f * K::hash (f.seed, i + 120)));
        g.fillEllipse (c.x + std::sin (a) * d - s * 0.5f, c.y - std::cos (a) * d - s * 0.5f, s, s);
    }
}

void epicycle (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r1 = f.S() * 0.28f, r2 = f.S() * 0.1f;
    juce::Path trace;
    const int steps = 220;
    for (int i = 0; i <= steps; ++i)
    {
        const float tt = f.t - (float) (steps - i) * 0.05f;
        const float x = c.x + std::cos (tt * 0.4f) * r1 + std::cos (tt * 2.2f) * r2;
        const float y = c.y + std::sin (tt * 0.4f) * r1 + std::sin (tt * 2.2f) * r2;
        if (i == 0) trace.startNewSubPath (x, y); else trace.lineTo (x, y);
    }
    K::glowLine (g, trace, k.line.withAlpha (0.75f), k.glow, 0.8f, 0.5f, k.light);
    K::halo (g, c, f.S() * 0.05f, k.line.withAlpha (0.8f));
    g.setColour (k.line.withAlpha (0.2f));
    g.drawEllipse (c.x - r1, c.y - r1, r1 * 2.0f, r1 * 2.0f, 0.6f);
}

void blackHole (G& g, const Frame& f)
{
    const auto& k = f.ink;
    const auto c = f.box.getCentre();
    const float r = f.H() * 0.22f;
    K::blob (g, c, r * 3.2f, r * 0.9f, k.glow.withAlpha (0.3f + 0.25f * f.bass));
    for (int i = 0; i < 4; ++i)
    {
        juce::Path disk;
        disk.addCentredArc (c.x, c.y, r * (1.6f + 0.35f * (float) i), r * (0.35f + 0.08f * (float) i), 0.0f, 0.0f, twoPi, true);
        g.setColour (k.accent.withAlpha (0.45f - 0.08f * (float) i));
        g.strokePath (disk, K::pen (1.0f));
    }
    juce::Path lens;
    lens.addCentredArc (c.x, c.y, r * 1.3f, r * 1.3f, 0.0f, -pi * 0.5f, pi * 0.5f, true);
    K::glowLine (g, lens, k.line, k.glow, 1.2f, 0.7f + f.bass, k.light);
    g.setColour (k.light ? k.sil : juce::Colours::black);
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
}
} // namespace

void addCelestial (std::vector<Spec>& v)
{
    const int hero = flagHero, both = flagHero | flagCompanion, comp = flagCompanion;
    const int sym = flagHero | flagNoFlip;
    //            name                    category              fn             zone            size  aspect  life           quiet bass  melod energ   flags
    v.push_back ({ "Crescent moon",        Category::Celestial, crescent,      Zone::Sky,      0.38f, 1.0f, 14.0f, 26.0f, { 0.9f, 0.2f, 0.7f, 0.1f }, both });
    v.push_back ({ "Full moon and cloud",  Category::Celestial, fullMoon,      Zone::Sky,      0.5f,  1.3f, 16.0f, 28.0f, { 0.9f, 0.3f, 0.6f, 0.1f }, hero });
    v.push_back ({ "Phases of the moon",   Category::Celestial, moonPhases,    Zone::Centre,   0.16f, 6.0f, 12.0f, 20.0f, { 0.6f, 0.3f, 0.7f, 0.3f }, sym });
    v.push_back ({ "Ringed planet",        Category::Celestial, ringedPlanet,  Zone::Anywhere, 0.5f,  1.6f, 14.0f, 24.0f, { 0.6f, 0.6f, 0.5f, 0.3f }, both });
    v.push_back ({ "Orrery",               Category::Celestial, orrery,        Zone::Centre,   0.62f, 1.6f, 12.0f, 22.0f, { 0.4f, 0.4f, 0.7f, 0.6f }, sym });
    v.push_back ({ "Eclipse",              Category::Celestial, eclipse,       Zone::Centre,   0.55f, 1.0f, 10.0f, 18.0f, { 0.3f, 0.9f, 0.3f, 0.7f }, sym });
    v.push_back ({ "Comet",                Category::Celestial, comet,         Zone::BandSky,  0.35f, 1.0f, 10.0f, 16.0f, { 0.6f, 0.3f, 0.7f, 0.4f }, comp });
    v.push_back ({ "Meteor shower",        Category::Celestial, meteorShower,  Zone::Sky,      0.5f,  2.0f, 10.0f, 18.0f, { 0.4f, 0.2f, 0.4f, 0.8f }, comp });
    v.push_back ({ "Orion",                Category::Celestial, orion,         Zone::Sky,      0.55f, 0.9f, 12.0f, 22.0f, { 0.8f, 0.3f, 0.6f, 0.2f }, sym | flagCompanion });
    v.push_back ({ "The Plough",           Category::Celestial, plough,        Zone::Sky,      0.45f, 1.6f, 12.0f, 22.0f, { 0.8f, 0.3f, 0.6f, 0.2f }, sym | flagCompanion });
    v.push_back ({ "Cassiopeia",           Category::Celestial, cassiopeia,    Zone::Sky,      0.4f,  1.6f, 12.0f, 22.0f, { 0.8f, 0.3f, 0.6f, 0.2f }, sym | flagCompanion });
    v.push_back ({ "The Northern Cross",   Category::Celestial, cygnus,        Zone::Sky,      0.55f, 0.9f, 12.0f, 22.0f, { 0.8f, 0.3f, 0.6f, 0.2f }, sym | flagCompanion });
    v.push_back ({ "Spiral galaxy",        Category::Celestial, galaxy,        Zone::Anywhere, 0.55f, 1.5f, 14.0f, 26.0f, { 0.6f, 0.5f, 0.7f, 0.4f }, hero });
    v.push_back ({ "Nebula",               Category::Celestial, nebula,        Zone::Anywhere, 0.65f, 1.6f, 16.0f, 30.0f, { 0.9f, 0.3f, 0.8f, 0.2f }, hero });
    v.push_back ({ "Astrolabe",            Category::Celestial, astrolabe,     Zone::Centre,   0.72f, 1.0f, 12.0f, 22.0f, { 0.4f, 0.5f, 0.6f, 0.6f }, sym });
    v.push_back ({ "Star chart",           Category::Celestial, starChart,     Zone::Centre,   0.72f, 1.0f, 12.0f, 22.0f, { 0.6f, 0.3f, 0.6f, 0.4f }, sym });
    v.push_back ({ "Wandering planet",     Category::Celestial, epicycle,      Zone::Centre,   0.62f, 1.0f, 12.0f, 22.0f, { 0.5f, 0.4f, 0.7f, 0.5f }, sym });
    v.push_back ({ "Black hole",           Category::Celestial, blackHole,     Zone::Centre,   0.5f,  1.8f, 10.0f, 18.0f, { 0.2f, 1.0f, 0.3f, 0.8f }, sym });
    juce::ignoreUnused (hero, both, comp);
}
}

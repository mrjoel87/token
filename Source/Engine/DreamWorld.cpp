#include "DreamWorld.h"
#include "../Util.h"
#include <algorithm>
#include <cstring>

namespace token::engine
{
namespace
{
    // the small sentimental pieces DREAM may show (by name from the artwork library)
    const char* const dreamPieces[] = {
        "Two on a bench", "Little aeroplane", "Rain cloud", "One lit window", "Lantern boat",
        "Night train", "Kite on the hill", "Watching the stars", "Cat on the rooftops", "Lighthouse",
        "Geese in formation", "Comet", "The telescope", "Under one umbrella", "Musician at the synth",
        "Crescent moon", "Figure by the lake",
    };

    bool isBand (art::Zone z) { return z == art::Zone::BandSky || z == art::Zone::BandMid || z == art::Zone::BandLow; }
}

float DreamWorld::Moment::alpha() const
{
    const float in = age / 2.5f;
    const float out = (life - age) / 3.0f;
    return fx::ease (juce::jmin (in, out, 1.0f));
}

DreamWorld::DreamWorld()
{
    rng.setSeedRandomly();
    const auto& lib = art::library();
    for (const char* name : dreamPieces)
        for (size_t i = 0; i < lib.size(); ++i)
            if (std::strcmp (lib[i].name, name) == 0)
                pool.push_back ((int) i);
}

float DreamWorld::horizonY (const SignalFrame& s, float u) const
{
    const auto A = s.area;
    const bool strip = A.getWidth() / juce::jmax (1.0f, A.getHeight()) > 1.9f;
    const float base = A.getY() + A.getHeight() * (strip ? 0.66f : 0.68f);
    const float hills = std::sin (u * 5.0f + s.evo * 0.03f) * 0.5f + std::sin (u * 11.0f - s.evo * 0.02f) * 0.25f;
    const float amp = A.getHeight() * (strip ? 0.08f : 0.06f);
    return base - (hills * 0.5f + 0.5f) * amp - fx::sampleAt (s.low, u) * amp * (0.6f + 0.8f * s.react) * (0.4f + s.bass);
}

void DreamWorld::startMoment (const SignalFrame& s)
{
    if (pool.empty())
        return;

    if (bag.empty())
    {
        bag = pool;
        for (int i = (int) bag.size() - 1; i > 0; --i)
            std::swap (bag[(size_t) i], bag[(size_t) rng.nextInt (i + 1)]);
    }

    // the musician is a secret: usually skipped
    int pick = bag.back();
    if (std::strcmp (art::library()[(size_t) pick].name, "Musician at the synth") == 0 && rng.nextFloat() > 0.15f && bag.size() > 1)
        std::swap (bag.back(), bag.front());
    pick = bag.back();
    bag.pop_back();

    moment = {};
    moment.spec = pick;
    moment.life = (14.0f + rng.nextFloat() * 8.0f) * (1.0f + 0.4f * s.quiet);
    moment.u = 0.2f + rng.nextFloat() * 0.6f;
    moment.flip = (art::library()[(size_t) pick].flags & art::flagNoFlip) == 0 && rng.nextBool();
    moment.seed = (juce::uint32) rng.nextInt (0x7fffffff);
    hasMoment = true;
}

void DreamWorld::update (const SignalFrame& s)
{
    if (hasMoment)
    {
        if (! s.locked)                       // LOCK keeps the current moment on screen
            moment.age += s.dt * juce::jmax (0.5f, s.speed);
        else
            moment.age = juce::jmin (moment.age, juce::jmax (2.5f, moment.life - 3.0f));
        if (moment.age >= moment.life)
        {
            hasMoment = false;
            gap = (14.0f + rng.nextFloat() * 18.0f) * (1.0f - 0.3f * s.quiet);   // sparing: these should feel special
        }
    }
    else if (! s.locked)
    {
        gap -= s.dt * s.speed;
        if (gap <= 0.0f)
            startMoment (s);
    }
}

void DreamWorld::draw (juce::Graphics& g, const SignalFrame& s, float alpha) const
{
    if (alpha <= 0.005f)
        return;

    const auto A = s.area;
    const auto& pal = s.pal;
    const int q = s.quality;
    const bool strip = A.getWidth() / juce::jmax (1.0f, A.getHeight()) > 1.9f;

    // sky light
    {
        const auto moonPos = juce::Point<float> (A.getX() + A.getWidth() * (0.15f + 0.7f * (0.5f + 0.5f * std::sin (s.evo * 0.01f))),
                                                 A.getY() + A.getHeight() * (strip ? 0.22f : 0.2f));
        const float r = A.getHeight() * (strip ? 0.07f : 0.045f);
        fx::halo (g, moonPos, r * 5.0f, pal.b.withAlpha ((0.08f + 0.08f * s.bass) * alpha));
        g.setColour (pal.a.interpolatedWith (juce::Colours::white, 0.5f).withAlpha (0.85f * alpha));
        g.fillEllipse (moonPos.x - r, moonPos.y - r, r * 2.0f, r * 2.0f);
    }

    // stars
    for (int i = 0; i < (q >= 1 ? 70 : 35); ++i)
    {
        const float x = A.getX() + fx::hash (5u, i * 3) * A.getWidth();
        const float y = A.getY() + fx::hash (5u, i * 3 + 1) * A.getHeight() * 0.55f;
        const float tw = 0.5f + 0.5f * std::sin (s.motion * (0.6f + fx::hash (5u, i + 400) * 1.5f) + (float) i);
        const float lit = juce::jlimit (0.0f, 1.0f, 0.25f + 0.4f * tw + 0.6f * s.hat * fx::hash (9u, i + (int) (s.motion * 4.0f)));
        const float size = 0.8f + 1.2f * fx::hash (5u, i * 3 + 2);
        g.setColour (pal.a.withAlpha (lit * alpha * 0.8f));
        g.fillRect (x, y, size, size);
    }

    // soft clouds thicken with sustained sound
    if (q >= 1)
    {
        for (int i = 0; i < 3; ++i)
        {
            const float u = std::fmod (fx::hash (3u, i) + s.motion * 0.004f * (1.0f + (float) i * 0.3f), 1.3f) - 0.15f;
            const juce::Point<float> c (A.getX() + u * A.getWidth(), A.getY() + A.getHeight() * (0.25f + 0.1f * (float) i));
            juce::Graphics::ScopedSaveState state (g);
            g.addTransform (juce::AffineTransform::scale (1.0f, 0.3f, c.x, c.y));
            fx::halo (g, c, A.getWidth() * 0.16f, pal.b.withAlpha ((0.05f + 0.1f * s.sustain) * alpha));
        }
    }

    // the horizon IS the waveform
    hills.clear();
    reflection.clear();
    const int n = q >= 2 ? 220 : 120;
    const float baseY = A.getY() + A.getHeight() * (strip ? 0.66f : 0.68f);
    for (int i = 0; i < n; ++i)
    {
        const float u = (float) i / (float) (n - 1);
        const float x = A.getX() + u * A.getWidth();
        const float y = horizonY (s, u);
        if (i == 0) { hills.startNewSubPath (x, y); reflection.startNewSubPath (x, baseY * 2.0f - y); }
        else        { hills.lineTo (x, y);          reflection.lineTo (x, baseY * 2.0f - y); }
    }

    auto ground = hills;
    ground.lineTo (A.getRight(), A.getBottom());
    ground.lineTo (A.getX(), A.getBottom());
    ground.closeSubPath();
    g.setColour (juce::Colour (0xff04050a).withAlpha (0.92f * alpha));
    g.fillPath (ground);

    g.setColour (pal.b.withAlpha (0.18f * alpha));
    g.strokePath (reflection, fx::pen (0.8f));
    fx::glow (g, hills, pal.a.withAlpha (0.8f * alpha), pal.b.withAlpha (alpha), 1.1f, 0.5f + 0.5f * s.bass, q);

    // glints of fine detail along the ridge, lit by the highs
    if (q >= 1 && s.high > 0.05f)
    {
        for (int i = 0; i < 40; ++i)
        {
            const float u = fx::hash (11u, i);
            const float glint = juce::jmax (0.0f, fx::sampleAt (s.fine, u)) * s.high * 3.0f;
            if (glint < 0.05f)
                continue;
            g.setColour (pal.c.withAlpha (juce::jlimit (0.0f, 1.0f, glint) * alpha));
            g.fillEllipse (A.getX() + u * A.getWidth() - 1.0f, horizonY (s, u) - 1.0f, 2.0f, 2.0f);
        }
    }

    // the moment
    if (! hasMoment || moment.spec < 0)
        return;

    const auto& spec = art::library()[(size_t) moment.spec];
    const float ma = moment.alpha() * alpha;
    if (ma <= 0.005f)
        return;

    juce::Rectangle<float> box;
    if (isBand (spec.zone))
    {
        const float h = A.getHeight() * (strip ? 0.3f : 0.16f);
        const float y = spec.zone == art::Zone::BandLow ? baseY - h * 0.8f : A.getY() + A.getHeight() * 0.08f;
        box = { A.getX(), y, A.getWidth(), h };
    }
    else
    {
        const float h = A.getHeight() * (strip ? 0.42f : 0.24f);
        const float w = juce::jmin (A.getWidth() * 0.4f, h * spec.aspect);
        if (spec.zone == art::Zone::Sky)
        {
            box = { A.getX() + moment.u * (A.getWidth() - w), A.getY() + A.getHeight() * 0.06f, w, h };
        }
        else
        {
            const float cx = A.getX() + moment.u * A.getWidth();
            const float groundY = horizonY (s, moment.u);
            box = juce::Rectangle<float> (w, h).withCentre ({ cx, groundY - h * 0.5f + h * 0.04f });
            box.setX (juce::jlimit (A.getX(), A.getRight() - w, box.getX()));
        }
    }

    art::Frame fr;
    fr.box = box;
    fr.t = moment.age;
    fr.life = juce::jlimit (0.0f, 1.0f, moment.age / moment.life);
    fr.px = juce::jmax (1.0f, std::round (A.getHeight() / 120.0f));
    fr.bass = s.bass; fr.mid = s.mid; fr.high = s.high; fr.kick = s.kick; fr.hat = s.hat;
    fr.energy = s.energy; fr.flow = s.sustain; fr.drop = s.drop;
    fr.beat = s.beatPhase; fr.beats = (float) std::fmod (s.beats, 4096.0); fr.bar = s.barPhase;
    fr.travel = s.evo * 0.5f; fr.speed = s.speed;
    fr.spectrum = s.audio != nullptr ? s.audio->spectrum.data() : nullptr;
    fr.bands = AudioFeatures::numSpectrumBands;
    fr.seed = moment.seed;
    fr.flip = moment.flip;
    fr.ink.line = pal.a;
    fr.ink.glow = pal.b;
    fr.ink.accent = pal.c;
    fr.ink.sil = juce::Colour (0xff05070c);
    fr.ink.haze = pal.b;
    fr.ink.warm = pal.c;
    fr.ink.bg = juce::Colour (0xff04050a);
    fr.ink.light = false;

    juce::Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (box.expanded (8.0f).getIntersection (A).toNearestInt());
    g.beginTransparencyLayer (juce::jmin (1.0f, ma));
    spec.draw (g, fr);
    g.endTransparencyLayer();
}
}

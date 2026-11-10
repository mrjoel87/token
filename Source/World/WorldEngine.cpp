#include "WorldEngine.h"
#include "PixelArt.h"
#include "../Palette.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace token
{
namespace
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    // sky colours per theme: Dream / Mystic / Norse
    const juce::Colour skyTop[3]     = { juce::Colour (0xff0b1634), juce::Colour (0xff1a0f2e), juce::Colour (0xff06202a) };
    const juce::Colour skyHorizon[3] = { juce::Colour (0xff23193d), juce::Colour (0xff3a2016), juce::Colour (0xff0b2c33) };
    const juce::Colour skylineFill[3]= { juce::Colour (0xff05080f), juce::Colour (0xff0a0608), juce::Colour (0xff040b0e) };
}

WorldEngine::WorldEngine()
    : library (createEventLibrary())
{
   #if defined (TOKEN_DEBUG_SEED)
    rng.setSeed (TOKEN_DEBUG_SEED);
   #else
    rng.setSeedRandomly();
   #endif

    juce::Random r (1234);   // the landscape itself is always the same

    for (int i = 0; i < 170; ++i)
        stars.push_back ({ r.nextFloat(), r.nextFloat(), 0.6f + 1.4f * r.nextFloat() * r.nextFloat(),
                           r.nextFloat() * twoPi, 0.2f + 0.8f * r.nextFloat(), r.nextFloat() });

    const int n = 160;
    for (auto& s : skylines)
        s.resize ((size_t) n);

    for (int i = 0; i < n; ++i)
    {
        const float fi = (float) i;
        skylines[0][(size_t) i] = 0.10f + 0.05f * std::sin (fi * 0.09f + 1.0f) + 0.035f * std::sin (fi * 0.23f + 2.0f) + 0.01f * r.nextFloat();
        skylines[1][(size_t) i] = 0.06f + 0.05f * std::pow (std::sin (fi * 0.05f), 2.0f) + 0.02f * std::sin (fi * 0.13f);
        skylines[2][(size_t) i] = 0.14f + 0.15f * std::abs (std::sin (fi * 0.07f + 0.5f)) + 0.08f * std::abs (std::sin (fi * 0.19f + 1.3f)) + 0.03f * r.nextFloat();
    }
}

int WorldEngine::maxEvents (const WorldContext& c) const
{
    if (minimal)
        return 1;

    int m = c.strip ? 2 : 3;
    if (c.chaos > 0.7f && c.chaosState() > 0.5f)
        ++m;
    return m;
}

// =================================================================================================
void WorldEngine::update (WorldContext& c, SceneMode scene)
{
    c.particles = &particles;
    c.rng = &rng;
    const auto& f = c.f();
    minimal = scene == SceneMode::Minimal;

    // ---- theme crossfade
    std::array<float, 3> target { 0.0f, 0.0f, 0.0f };
    switch (scene)
    {
        case SceneMode::Auto:
            autoThemeTimer -= c.dt;
            if (autoThemeTimer <= 0.0f)
            {
                autoTheme = (autoTheme + 1 + rng.nextInt (2)) % 3;
                autoThemeTimer = 70.0f + rng.nextFloat() * 50.0f;
            }
            target[(size_t) autoTheme] = 1.0f;
            break;
        case SceneMode::Minimal:
        case SceneMode::Dream:  target[0] = 1.0f; break;
        case SceneMode::Mystic: target[1] = 1.0f; break;
        case SceneMode::Norse:  target[2] = 1.0f; break;
    }

    for (size_t i = 0; i < 3; ++i)
        theme[i] = smoothTo (theme[i], target[i], c.dt, 4.0f);
    c.theme = theme;

    scroll += c.dt * (4.0f + 14.0f * f.intensity * c.react);
    auroraPhase += c.dt * (0.15f + 0.4f * f.sustain);

    // ---- kick shockwaves
    if (f.kickHit && ! minimal)
    {
        shocks[(size_t) shockHead] = { 0.0f, juce::jlimit (0.0f, 1.0f, f.kick * c.reactScale()) };
        shockHead = (shockHead + 1) % (int) shocks.size();
    }
    for (auto& s : shocks)
        s.age += c.dt;

    spawnAmbient (c);

    // ---- events
    for (auto& e : active)
        e->advance (c);

    active.erase (std::remove_if (active.begin(), active.end(),
                                  [] (const std::unique_ptr<WorldEvent>& e) { return e->isFinished(); }),
                  active.end());

    schedule (c);

    const float wind = 6.0f + 20.0f * f.intensity;
    particles.update (c.dt, c.screen, wind);
}

void WorldEngine::spawnAmbient (const WorldContext& c)
{
    const auto& f = c.f();
    const auto& s = c.screen;
    const float px = c.px;
    const float k = minimal ? 0.3f : 1.0f;

    // sparkles riding the waveform (high frequencies)
    sparkleAcc += c.dt * (25.0f * c.high() + 40.0f * c.snare()) * k;
    while (sparkleAcc >= 1.0f)
    {
        sparkleAcc -= 1.0f;
        const float x = s.getX() + rng.nextFloat() * s.getWidth();
        const float y = c.scope->waveYAt (x) + (rng.nextFloat() - 0.5f) * 12.0f * px;
        const auto col = rng.nextFloat() < 0.8f ? pal::cyan.interpolatedWith (pal::moon, 0.5f) : pal::pink;
        particles.spawn (ParticleType::Spark, { x, y }, { (rng.nextFloat() - 0.5f) * 30.0f, -10.0f - rng.nextFloat() * 20.0f },
                         0.5f + rng.nextFloat() * 0.6f, px * (0.4f + 0.4f * rng.nextFloat()), col);
    }

    // snare-like burst from a waveform peak
    if (f.snareHit && ! minimal)
    {
        const float x = s.getX() + s.getWidth() * (0.1f + 0.8f * rng.nextFloat());
        const float y = c.scope->envelopeTopAt (x);
        const int n = 5 + (int) (12.0f * c.snare());
        for (int i = 0; i < n; ++i)
        {
            const float a = rng.nextFloat() * twoPi;
            const float v = 40.0f + 90.0f * rng.nextFloat();
            particles.spawn (ParticleType::Spark, { x, y }, { std::cos (a) * v, std::sin (a) * v },
                             0.5f + 0.5f * rng.nextFloat(), px * 0.5f, rng.nextBool() ? pal::gold : pal::moon);
        }
    }

    // dream dust
    dustAcc += c.dt * 3.0f * theme[0] * (1.0f - f.stateWeights[2]) * k;
    while (dustAcc >= 1.0f)
    {
        dustAcc -= 1.0f;
        particles.spawn (ParticleType::Dust, { s.getX() + rng.nextFloat() * s.getWidth(), s.getY() + rng.nextFloat() * s.getHeight() },
                         { 3.0f + rng.nextFloat() * 6.0f, -2.0f + rng.nextFloat() * 4.0f }, 5.0f + rng.nextFloat() * 4.0f,
                         px * 0.6f, pal::moon.withAlpha (0.35f));
    }

    // norse snow
    snowAcc += c.dt * 16.0f * theme[2] * (0.35f + 0.65f * f.sustain) * k;
    while (snowAcc >= 1.0f)
    {
        snowAcc -= 1.0f;
        particles.spawn (ParticleType::Snow, { s.getX() + rng.nextFloat() * s.getWidth(), s.getY() - 2.0f },
                         { 0.0f, 12.0f + rng.nextFloat() * 22.0f }, s.getHeight() / 12.0f,
                         juce::jmax (1.0f, px * (0.4f + 0.4f * rng.nextFloat())), pal::moon.withAlpha (0.6f));
    }

    // mystic sand drifting on the wind
    sandAcc += c.dt * 6.0f * theme[1] * k;
    while (sandAcc >= 1.0f)
    {
        sandAcc -= 1.0f;
        particles.spawn (ParticleType::Dust, { s.getX(), s.getBottom() - rng.nextFloat() * s.getHeight() * 0.4f },
                         { 30.0f + rng.nextFloat() * 40.0f, -3.0f + rng.nextFloat() * 6.0f }, 6.0f,
                         px * 0.5f, pal::amber.withAlpha (0.4f));
    }

    // chaos embers
    emberAcc += c.dt * 28.0f * f.stateWeights[2] * c.reactScale() * k;
    while (emberAcc >= 1.0f)
    {
        emberAcc -= 1.0f;
        particles.spawn (ParticleType::Ember, { s.getX() + rng.nextFloat() * s.getWidth(), s.getBottom() + 2.0f },
                         { (rng.nextFloat() - 0.5f) * 20.0f, -30.0f - rng.nextFloat() * 60.0f }, 2.0f + rng.nextFloat(),
                         px * 0.7f, rng.nextBool() ? pal::amber : pal::crimson);
    }
}

// =================================================================================================
//  Scheduler: weighted randomness shaped by audio intensity, energy state, theme, rarity, cooldowns
// =================================================================================================
void WorldEngine::schedule (const WorldContext& c)
{
    const auto& f = c.f();

    // musical moments that call for a specific scene
    if (f.stateChanged)
    {
        if (f.state == EnergyState::Chaos)
        {
            if (! trySpawn ("eye-pyramid", c))
                trySpawn ("rune", c);
        }
        else if (f.previousState == EnergyState::Chaos)
        {
            trySpawn (theme[2] > 0.3f ? "ravens" : "shooting-star", c);
        }
        else if (f.state == EnergyState::Dream)
        {
            trySpawn (rng.nextBool() ? "aeroplane" : "bench", c);
        }
    }

    if (f.snareHit && rng.nextFloat() < 0.04f * c.chaos)
        trySpawn ("shooting-star", c);

    rollTimer -= c.dt;
    if (rollTimer > 0.0f)
        return;

    rollTimer = 0.5f + rng.nextFloat() * 0.6f;

    if ((int) active.size() >= maxEvents (c))
        return;

    const float minGap = juce::jmap (c.chaos, 6.0f, 1.2f);
    if (c.time - lastSpawnTime < minGap)
        return;

    const float probability = (0.15f + 0.55f * c.chaos) * (0.55f + 0.7f * f.intensity);
    if (rng.nextFloat() > probability)
        return;

    const int index = pickWeighted (c);
    if (index >= 0)
        spawn (index, c);
}

int WorldEngine::pickWeighted (const WorldContext& c)
{
    const auto& sw = c.f().stateWeights;
    weightScratch.assign (library.size(), 0.0f);
    float total = 0.0f;

    for (size_t i = 0; i < library.size(); ++i)
    {
        const auto& d = library[i];

        if (minimal && ! d.allowMinimal)            continue;
        if (c.time - d.lastSpawn < d.cooldown)      continue;

        bool isActive = false;
        for (const auto& e : active)
            isActive = isActive || e->defIndex == (int) i;
        if (isActive)
            continue;

        float rarity = 1.0f;
        switch (d.rarity)
        {
            case Rarity::Common:   rarity = 1.0f; break;
            case Rarity::Uncommon: rarity = 0.45f; break;
            case Rarity::Rare:     rarity = 0.2f; break;
            case Rarity::VeryRare: rarity = 0.035f * (0.5f + c.chaos); break;
            case Rarity::Secret:   rarity = 0.01f * (0.5f + c.chaos); break;
        }

        float themeW = 0.06f;   // small chance of a scene from another world: unusual combinations
        for (int k = 0; k < 3; ++k)
            if ((d.themes & (1 << k)) != 0)
                themeW += theme[(size_t) k];

        const float stateW = sw[0] * d.wDream + sw[1] * d.wPulse + sw[2] * d.wChaos;
        const float w = rarity * themeW * stateW;
        weightScratch[i] = w;
        total += w;
    }

    if (total <= 0.0f)
        return -1;

    float pick = rng.nextFloat() * total;
    for (size_t i = 0; i < weightScratch.size(); ++i)
    {
        if (weightScratch[i] <= 0.0f)
            continue;
        pick -= weightScratch[i];
        if (pick <= 0.0f)
            return (int) i;
    }
    return -1;
}

bool WorldEngine::trySpawn (const char* name, const WorldContext& c)
{
    if (minimal || (int) active.size() > maxEvents (c))
        return false;

    for (int i = 0; i < (int) library.size(); ++i)
    {
        const auto& d = library[(size_t) i];
        if (std::strcmp (d.name, name) != 0)
            continue;

        if (c.time - d.lastSpawn < d.cooldown * 0.5f)
            return false;

        for (const auto& e : active)
            if (e->defIndex == i)
                return false;

        spawn (i, c);
        return true;
    }
    return false;
}

void WorldEngine::spawn (int index, const WorldContext& c)
{
    auto& d = library[(size_t) index];
    auto e = d.make (c);
    if (e == nullptr)
        return;

    e->defIndex = index;
    active.push_back (std::move (e));
    d.lastSpawn = c.time;
    lastSpawnTime = c.time;
}

// =================================================================================================
//  Drawing
// =================================================================================================
void WorldEngine::drawBackground (juce::Graphics& g, const WorldContext& c) const
{
    const auto s = c.screen;
    const auto& f = c.f();
    const float dim = minimal ? 0.45f : 1.0f;

    const auto top = pal::mix3 (theme, skyTop[0], skyTop[1], skyTop[2]).interpolatedWith (pal::black, 1.0f - dim);
    const auto horizon = pal::mix3 (theme, skyHorizon[0], skyHorizon[1], skyHorizon[2]).interpolatedWith (pal::black, 1.0f - dim);

    juce::ColourGradient sky (top, 0.0f, s.getY(), pal::black, 0.0f, s.getBottom(), false);
    sky.addColour (0.72, horizon);
    g.setGradientFill (sky);
    g.fillRect (s);

    // deep bass pulse
    {
        const float b = c.bass();
        const auto centre = s.getCentre();
        juce::ColourGradient pulse (pal::cyan.withAlpha (0.06f * b * dim), centre.x, centre.y,
                                    pal::cyan.withAlpha (0.0f), centre.x + s.getWidth() * 0.5f, centre.y, true);
        g.setGradientFill (pulse);
        g.fillRect (s);
    }

    // stars
    const float starBoost = 0.7f + 0.6f * c.high();
    for (const auto& st : stars)
    {
        const float x = s.getX() + positiveMod (st.x * s.getWidth() - scroll * st.depth * 0.4f, s.getWidth());
        const float y = s.getY() + st.y * s.getHeight() * 0.75f;
        const float tw = 0.55f + 0.45f * std::sin (c.time * (0.8f + st.depth * 2.0f) + st.phase);
        float bright = tw * (0.3f + 0.5f * (st.size - 0.6f) / 1.4f) * starBoost;
        bright += (st.flashy > 0.85f ? c.snare() : 0.0f);
        const float sz = st.size * (c.strip ? 0.8f : 1.0f);

        g.setColour (pal::moon.withAlpha (juce::jlimit (0.0f, 1.0f, bright * 0.8f * dim)));
        g.fillRect (x, y, sz, sz);

        if (st.size > 1.55f)
        {
            g.setColour (pal::moon.withAlpha (juce::jlimit (0.0f, 1.0f, bright * 0.25f * dim)));
            g.fillRect (x - sz * 1.5f, y + sz * 0.4f, sz * 4.0f, sz * 0.2f);
            g.fillRect (x + sz * 0.4f, y - sz * 1.5f, sz * 0.2f, sz * 4.0f);
        }
    }

    if (! minimal)
    {
        drawMoon (g, c, top);

        const float auroraAmount = theme[2] * (0.35f + 0.65f * f.sustain + 0.2f * c.bass());
        if (auroraAmount > 0.01f)
            drawAurora (g, c, auroraAmount);

        for (int k = 0; k < 3; ++k)
            if (theme[(size_t) k] > 0.01f)
                drawSkyline (g, c, k, theme[(size_t) k]);
    }

    drawGraticule (g, c);

    // CHAOS: slow sacred-geometry hexagram behind everything
    const float chaosW = f.stateWeights[2];
    if (chaosW > 0.02f && ! minimal)
    {
        const auto centre = juce::Point<float> (s.getCentreX(), c.scope->centreY());
        const float R = s.getHeight() * 0.42f;
        const float rot = c.time * 0.1f;
        juce::Path hex;

        for (int t = 0; t < 2; ++t)
        {
            for (int i = 0; i <= 3; ++i)
            {
                const float a = rot + (float) t * juce::MathConstants<float>::pi + (float) i * twoPi / 3.0f;
                const juce::Point<float> p (centre.x + std::sin (a) * R, centre.y - std::cos (a) * R);
                if (i == 0) hex.startNewSubPath (p); else hex.lineTo (p);
            }
        }
        hex.addEllipse (centre.x - R * 1.05f, centre.y - R * 1.05f, R * 2.1f, R * 2.1f);

        g.setColour (pal::gold.withAlpha (0.07f * chaosW * (0.5f + c.glow)));
        g.strokePath (hex, juce::PathStrokeType (1.0f));
    }
}

void WorldEngine::drawMoon (juce::Graphics& g, const WorldContext& c, juce::Colour skyTopColour) const
{
    const float w = theme[0] + theme[1];
    if (w < 0.01f)
        return;

    const auto& s = c.screen;
    const float cx = s.getRight() - s.getWidth() * 0.14f;
    const float cy = s.getY() + s.getHeight() * 0.26f;
    const float r = s.getHeight() * (c.strip ? 0.12f : 0.07f);
    const auto colour = pal::mix3 ({ theme[0], theme[1], 0.0f }, pal::moon, pal::pink.interpolatedWith (pal::gold, 0.35f), pal::moon);

    juce::ColourGradient halo (colour.withAlpha (0.16f * w * (1.0f + 0.5f * c.bass())), cx, cy,
                               colour.withAlpha (0.0f), cx + r * 5.0f, cy, true);
    g.setGradientFill (halo);
    g.fillEllipse (cx - r * 5.0f, cy - r * 5.0f, r * 10.0f, r * 10.0f);

    g.setColour (colour.withAlpha (0.9f * w));
    g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);

    g.setColour (juce::Colours::black.withAlpha (0.07f * w));
    g.fillEllipse (cx - r * 0.5f, cy - r * 0.3f, r * 0.5f, r * 0.45f);
    g.fillEllipse (cx + r * 0.1f, cy + r * 0.2f, r * 0.35f, r * 0.3f);
    g.fillEllipse (cx + r * 0.2f, cy - r * 0.55f, r * 0.25f, r * 0.22f);

    // Mystic: crescent
    if (theme[1] > 0.01f)
    {
        g.setColour (skyTopColour.withAlpha (0.9f * theme[1]));
        g.fillEllipse (cx - r * 0.55f, cy - r * 1.2f, r * 2.0f, r * 2.0f);
    }
}

void WorldEngine::drawAurora (juce::Graphics& g, const WorldContext& c, float amount) const
{
    const auto& s = c.screen;
    const float step = juce::jmax (4.0f, s.getWidth() / 120.0f);

    for (int band = 0; band < 3; ++band)
    {
        const float fb = (float) band;
        const float baseY = s.getY() + s.getHeight() * (0.10f + 0.06f * fb);
        const float bandH = s.getHeight() * (0.24f - 0.04f * fb);

        juce::Path ribbon;
        std::vector<float> tops;

        for (float x = s.getX(); x <= s.getRight() + step; x += step)
        {
            const float y = baseY + std::sin (x * 0.006f + auroraPhase * (1.0f + fb * 0.3f) + fb * 2.0f) * s.getHeight() * 0.06f
                                  + std::sin (x * 0.017f - auroraPhase * 1.7f) * s.getHeight() * 0.025f;
            tops.push_back (y);
            if (ribbon.isEmpty()) ribbon.startNewSubPath (x, y); else ribbon.lineTo (x, y);
        }

        int i = (int) tops.size() - 1;
        for (float x = s.getX() + step * (float) i; i >= 0; --i, x -= step)
        {
            const float h = bandH * (0.7f + 0.3f * std::sin (x * 0.011f + auroraPhase * 2.0f + fb));
            ribbon.lineTo (x, tops[(size_t) i] + h);
        }
        ribbon.closeSubPath();

        const float a = juce::jlimit (0.0f, 1.0f, amount * 0.22f);
        juce::ColourGradient grad (pal::violet.withAlpha (0.0f), 0.0f, baseY - s.getHeight() * 0.08f,
                                   pal::turquoise.withAlpha (a), 0.0f, baseY + bandH, false);
        grad.addColour (0.5, pal::violet.withAlpha (a * 0.35f));
        g.setGradientFill (grad);
        g.fillPath (ribbon);
    }
}

void WorldEngine::drawSkyline (juce::Graphics& g, const WorldContext& c, int k, float weight) const
{
    const auto& s = c.screen;
    const auto& hm = skylines[(size_t) k];
    const int size = (int) hm.size();
    const float px = c.px;
    const float depth = k == 2 ? 0.5f : 0.35f;
    const float step = px * 2.0f;
    const int n = (int) std::ceil (s.getWidth() / step) + 2;
    const float offset = scroll * depth;
    const float breathe = 1.0f + 0.05f * c.bass();
    const float scaleH = s.getHeight();
    const int base = (int) std::floor (offset / step);
    const float x0 = s.getX() - positiveMod (offset, step);

    juce::Path p;
    p.startNewSubPath (s.getX(), s.getBottom());
    juce::RectangleList<float> caps;
    std::vector<juce::Point<float>> landmarks;

    for (int i = 0; i < n; ++i)
    {
        const int gi = ((base + i) % size + size) % size;
        const float hv = hm[(size_t) gi];
        const float y = std::round ((s.getBottom() - hv * scaleH * breathe) / px) * px;
        const float x = x0 + (float) i * step;

        p.lineTo (x, y);
        p.lineTo (x + step, y);

        if (k == 2 && hv > 0.30f)
            caps.addWithoutMerging ({ x, y, step, px });
        if ((k == 0 && gi == 37) || (k == 1 && (gi == 50 || gi == 118)))
            landmarks.push_back ({ x, y });
    }

    p.lineTo (s.getRight() + step, s.getBottom());
    p.closeSubPath();

    g.setColour (skylineFill[k].withAlpha (weight));
    g.fillPath (p);

    if (k == 2)
    {
        g.setColour (juce::Colour (0xff9fb4c4).withAlpha (0.45f * weight));
        g.fillRectList (caps);
    }

    for (const auto& lm : landmarks)
    {
        if (k == 0)
        {
            // a lonely house with one illuminated window
            g.setColour (skylineFill[0].withAlpha (weight));
            g.fillRect (lm.x, lm.y - 3.0f * px, 4.0f * px, 3.0f * px);
            juce::Path roof;
            roof.addTriangle (lm.x - px, lm.y - 3.0f * px, lm.x + 5.0f * px, lm.y - 3.0f * px, lm.x + 2.0f * px, lm.y - 5.0f * px);
            g.fillPath (roof);
            const float flick = 0.85f + 0.15f * std::sin (c.time * 0.7f);
            g.setColour (pal::amber.withAlpha (0.9f * weight * flick));
            g.fillRect (lm.x + 1.5f * px, lm.y - 2.0f * px, px, px);
            g.setColour (pal::amber.withAlpha (0.15f * weight * flick));
            g.fillEllipse (lm.x - 1.0f * px, lm.y - 4.5f * px, 6.0f * px, 6.0f * px);
        }
        else
        {
            // distant pyramids whose edges catch the bass
            const float h = 9.0f * px, w = 14.0f * px;
            juce::Path tri;
            tri.addTriangle (lm.x - w * 0.5f, lm.y, lm.x + w * 0.5f, lm.y, lm.x, lm.y - h);
            g.setColour (skylineFill[1].withAlpha (weight));
            g.fillPath (tri);
            juce::Path edges;
            edges.startNewSubPath (lm.x - w * 0.5f, lm.y);
            edges.lineTo (lm.x, lm.y - h);
            edges.lineTo (lm.x + w * 0.5f, lm.y);
            g.setColour (pal::gold.withAlpha (weight * (0.15f + 0.6f * c.bass())));
            g.strokePath (edges, juce::PathStrokeType (1.0f));
        }
    }
}

void WorldEngine::drawGraticule (juce::Graphics& g, const WorldContext& c) const
{
    const auto& s = c.screen;
    const auto a = c.scope->getArea();
    const float boost = minimal ? 1.4f : 1.0f;
    const int divX = c.strip ? 20 : 10;
    const int divY = c.strip ? 4 : 8;

    g.setColour (pal::cyan.withAlpha (0.035f * boost));
    for (int i = 1; i < divX; ++i)
        g.fillRect (juce::roundToInt (a.getX() + a.getWidth() * (float) i / (float) divX), (int) s.getY(), 1, (int) s.getHeight());
    for (int i = 1; i < divY; ++i)
        g.fillRect ((int) s.getX(), juce::roundToInt (s.getY() + s.getHeight() * (float) i / (float) divY), (int) s.getWidth(), 1);

    const int cy = juce::roundToInt (c.scope->centreY());
    g.setColour (pal::cyan.withAlpha (0.07f * boost));
    g.fillRect ((int) s.getX(), cy, (int) s.getWidth(), 1);

    for (int i = 0; i <= 50; ++i)
    {
        const int x = juce::roundToInt (a.getX() + a.getWidth() * (float) i / 50.0f);
        const int len = i % 5 == 0 ? 5 : 2;
        g.fillRect (x, cy - len, 1, len * 2 + 1);
    }
}

void WorldEngine::drawMidEffects (juce::Graphics& g, const WorldContext& c) const
{
    const auto& s = c.screen;
    const float cx = s.getCentreX(), cy = c.scope->centreY();

    for (const auto& sh : shocks)
    {
        if (sh.age >= 0.9f || sh.strength <= 0.0f)
            continue;

        const float t = sh.age / 0.9f;
        const float rx = s.getWidth() * (0.05f + 0.55f * t);
        const float ry = s.getHeight() * (0.05f + 0.40f * t);
        const float alpha = (1.0f - t) * (1.0f - t) * 0.35f * sh.strength * (0.4f + c.glow);
        g.setColour (pal::cyan.withAlpha (juce::jlimit (0.0f, 1.0f, alpha)));
        g.drawEllipse (cx - rx, cy - ry, rx * 2.0f, ry * 2.0f, 1.0f + 3.0f * (1.0f - t));
    }
}

void WorldEngine::drawEvents (juce::Graphics& g, const WorldContext& c, Layer layer) const
{
    for (const auto& e : active)
        if (e->layer == layer)
            e->draw (g, c, e->envelope());
}

void WorldEngine::drawParticles (juce::Graphics& g) const
{
    particles.draw (g);
}
}

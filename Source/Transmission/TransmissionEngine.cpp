#include "TransmissionEngine.h"
#include "../Palette.h"
#include "../Util.h"
#include <algorithm>
#include <cmath>

namespace token::tx
{
namespace
{
    const char* categoryName (Category c)
    {
        switch (c)
        {
            case Category::Lost:   return "LOST";
            case Category::Occult: return "OCCULT";
            case Category::Human:  return "HUMAN";
            case Category::Nature: return "NATURE";
            case Category::Roads:  return "ROADS";
        }
        return "";
    }

    float barsForSync (SyncMode s)
    {
        switch (s)
        {
            case SyncMode::Quarter: return 0.25f;
            case SyncMode::Half:    return 0.5f;
            case SyncMode::One:     return 1.0f;
            case SyncMode::Two:     return 2.0f;
            case SyncMode::Four:    return 4.0f;
            case SyncMode::Eight:   return 8.0f;
            case SyncMode::Sixteen: return 16.0f;
            case SyncMode::Free:
            case SyncMode::Auto:    break;
        }
        return 4.0f;
    }
}

TransmissionEngine::TransmissionEngine()
{
   #if defined (TOKEN_DEBUG_SEED)
    rng.setSeed (TOKEN_DEBUG_SEED + 1);
   #else
    rng.setSeedRandomly();
   #endif
    slots.reserve (6);
}

Ink TransmissionEngine::inkFromPalette()
{
    Ink k;
    if (pal::lightScreen)
    {
        k.deep   = pal::black;
        k.haze   = juce::Colour (0xfff7f7f5);
        k.sil    = juce::Colour (0xff1b1d21);
        k.mid    = juce::Colour (0xff7d8085);
        k.light  = juce::Colour (0xff2c3036);
        k.accent = pal::cyan;
        k.warm   = juce::Colour (0xff4b4642);
    }
    else
    {
        k.deep   = pal::black;
        k.haze   = pal::midnight.interpolatedWith (pal::moon, 0.22f);
        k.sil    = pal::black.interpolatedWith (juce::Colours::black, 0.6f);
        k.mid    = pal::midnight.interpolatedWith (pal::moon, 0.38f);
        k.light  = pal::moon;
        k.accent = pal::cyan;
        k.warm   = pal::amber;
    }
    return k;
}

float TransmissionEngine::Slot::alpha() const noexcept
{
    const float in = fadeIn > 0.0f ? age / fadeIn : 1.0f;
    const float out = leaving ? 1.0f - leaveAge / juce::jmax (0.01f, fadeOut) : 1.0f;
    return smoothstep (juce::jmin (in, out, 1.0f));
}

// =================================================================================================
//  Timing
// =================================================================================================
float TransmissionEngine::autoBars (const TxContext& c) const
{
    float bars = 4.0f;
    if (profile[4] > 0.5f)       bars = 2.0f;
    else if (profile[1] > 0.5f)  bars = 4.0f;
    else if (profile[0] > 0.45f) bars = 8.0f;

    switch (c.settings.speed)
    {
        case SpeedMode::Slow: bars *= 2.0f; break;
        case SpeedMode::Fast: bars *= 0.5f; break;
        case SpeedMode::Normal:
        case SpeedMode::Auto: break;
    }
    return juce::jlimit (1.0f, 16.0f, bars);
}

float TransmissionEngine::intervalSeconds (const TxContext& c) const
{
    const auto& tr = c.transport;
    const bool barSync = tr.playing && c.settings.sync != SyncMode::Free;

    if (barSync)
        return (float) ((double) currentBars * tr.beatsPerBar * 60.0 / juce::jmax (20.0, tr.bpm));

    switch (c.settings.speed)
    {
        case SpeedMode::Slow:   return 26.0f;
        case SpeedMode::Normal: return 14.0f;
        case SpeedMode::Fast:   return 7.0f;
        case SpeedMode::Auto:   break;
    }
    return 22.0f - 14.0f * juce::jlimit (0.0f, 1.0f, profile[1] * 0.6f + profile[4] * 0.7f);
}

// =================================================================================================
//  Selection
// =================================================================================================
int TransmissionEngine::chooseScene (const TxContext& c, bool apparition)
{
    const auto& lib = sceneLibrary();
    const auto& s = c.settings;

    for (int attempt = 0; attempt < 2; ++attempt)
    {
        weights.assign (lib.size(), 0.0f);
        float total = 0.0f;
        int eligible = 0;

        for (size_t i = 0; i < lib.size(); ++i)
        {
            const auto& spec = lib[i];

            if (apparition)
            {
                if (spec.category != Category::Occult)
                    continue;
            }
            else
            {
                if (! s.categories[(size_t) spec.category])
                    continue;
                if (spec.rare && ! s.rare)
                    continue;
            }

            ++eligible;
            if (std::find (history.begin(), history.end(), (int) i) != history.end())
                continue;

            const auto& a = spec.affinity;
            float score = 0.2f + a.ambient * profile[0] + a.rhythm * profile[1] + a.sparse * profile[2]
                               + a.dark * profile[3] + a.intense * profile[4];

            if (spec.category == Category::Occult) score *= 0.5f + s.mystery * 1.2f;
            else                                   score *= 1.15f - s.mystery * 0.3f;

            if (spec.rare)
                score *= 0.25f * (0.5f + s.mystery);

            weights[i] = score * score;
            total += weights[i];
        }

        if (total > 0.0f)
        {
            float pick = rng.nextFloat() * total;
            for (size_t i = 0; i < weights.size(); ++i)
            {
                if (weights[i] <= 0.0f)
                    continue;
                pick -= weights[i];
                if (pick <= 0.0f)
                {
                    if (! apparition)
                    {
                        history.push_back ((int) i);
                        const size_t cap = (size_t) juce::jlimit (1, 14, eligible / 2);
                        while (history.size() > cap)
                            history.erase (history.begin());
                    }
                    return (int) i;
                }
            }
        }

        if (eligible == 0)
            return -1;

        history.clear();   // everything eligible was used recently - start the cycle again
    }

    return -1;
}

void TransmissionEngine::startScene (int index, const TxContext& c, float fade, bool apparition, float life)
{
    if (! apparition)
        for (auto& sl : slots)
            if (! sl.leaving && ! sl.apparition)
            {
                sl.leaving = true;
                sl.leaveAge = 0.0f;
                sl.fadeOut = fade;
            }

    while (slots.size() >= 5)
        slots.erase (slots.begin());

    Slot sl;
    sl.scene = index;
    sl.seed = (juce::uint32) rng.nextInt (0x7fffffff);
    sl.fadeIn = apparition ? 0.08f : fade;
    sl.fadeOut = apparition ? 0.3f : fade;
    sl.life = life;
    sl.apparition = apparition;
    slots.push_back (std::move (sl));

    if (! apparition)
    {
        const auto& spec = sceneLibrary()[(size_t) index];
        status = juce::String::formatted ("RX %03d  /  ", index + 1) + categoryName (spec.category);
    }
    juce::ignoreUnused (c);
}

// =================================================================================================
//  Update
// =================================================================================================
void TransmissionEngine::update (const TxContext& c)
{
    const auto& f = *c.audio;
    const auto& s = c.settings;

    if (s.visibility <= 0.001f)
    {
        slots.clear();
        status = {};
        started = false;
        return;
    }

    // ---- musical profile (slow, to avoid abrupt or predictable changes)
    const float onset = f.onsetRate;
    const std::array<float, 5> target {
        juce::jlimit (0.0f, 1.0f, f.sustain * (1.0f - juce::jlimit (0.0f, 1.0f, onset / 4.0f)) + 0.35f * (1.0f - f.intensity)),
        juce::jlimit (0.0f, 1.0f, juce::jlimit (0.0f, 1.0f, onset / 5.0f) * (0.4f + 0.6f * (f.stateWeights[1] + f.stateWeights[2]))),
        juce::jlimit (0.0f, 1.0f, (1.0f - f.loudness) * (1.0f - juce::jlimit (0.0f, 1.0f, onset / 3.0f))),
        juce::jlimit (0.0f, 1.0f, f.bass - f.high * 0.8f + 0.35f * (1.0f - f.centroid)),
        juce::jlimit (0.0f, 1.0f, f.stateWeights[2] * 0.75f + f.spectralMotion * 0.3f)
    };
    for (size_t i = 0; i < profile.size(); ++i)
        profile[i] = smoothTo (profile[i], target[i], c.dt, 3.0f);

    // ---- beat clock: host transport when playing, otherwise a gentle 120 BPM
    if (c.transport.playing)
        beatNow = (float) std::fmod (c.transport.ppq, 4096.0);
    else
        beatNow = (freeBeat += c.dt * 2.0f);

    // ---- when to change
    sinceChange += c.dt;
    const bool barSync = c.transport.playing && s.sync != SyncMode::Free;

    if (s.sync != SyncMode::Auto && s.sync != SyncMode::Free)
        currentBars = barsForSync (s.sync);

    bool change = false;

    if (! started)
    {
        change = c.time > 1.0f;
    }
    else if (barSync)
    {
        const double unit = (double) currentBars * c.transport.beatsPerBar;
        const double idx = std::floor (c.transport.ppq / unit);

        if (unit != lastUnit)
        {
            lastUnit = unit;
            lastBarIndex = idx;
        }
        else if (idx != lastBarIndex)
        {
            lastBarIndex = idx;
            change = sinceChange > 0.25f;
        }
    }
    else
    {
        change = sinceChange >= intervalSeconds (c);
    }

    if (change)
    {
        const int idx = chooseScene (c, false);
        if (idx >= 0)
        {
            const float fade = juce::jlimit (0.35f, 4.5f, intervalSeconds (c) * 0.35f);
            startScene (idx, c, fade, false, 1.0e9f);
            sinceChange = 0.0f;
            started = true;

            if (s.sync == SyncMode::Auto)
            {
                currentBars = autoBars (c);
                lastUnit = -1.0;   // re-anchor the bar grid to the new length
            }
        }
        else if (! started)
        {
            status = {};
        }
    }

    // ---- rare apparitions: brief glimpses from the Occult library
    if (s.rare)
    {
        apparitionTimer -= c.dt * (0.4f + s.density) * (0.3f + 1.4f * s.mystery);
        const bool snareGlimpse = f.snareHit && rng.nextFloat() < 0.015f * s.mystery;

        if (apparitionTimer <= 0.0f || snareGlimpse)
        {
            apparitionTimer = 6.0f + rng.nextFloat() * 14.0f;
            const int idx = chooseScene (c, true);
            if (idx >= 0)
                startScene (idx, c, 0.1f, true, 0.25f + rng.nextFloat() * 0.7f);
        }
    }

    // ---- advance, render, release particles
    for (auto& sl : slots)
    {
        sl.age += c.dt;
        if (sl.apparition && ! sl.leaving && sl.age > sl.life)
        {
            sl.leaving = true;
            sl.leaveAge = 0.0f;
        }
        if (sl.leaving)
            sl.leaveAge += c.dt;
    }

    slots.erase (std::remove_if (slots.begin(), slots.end(), [] (const Slot& sl) { return sl.finished(); }), slots.end());

    for (auto& sl : slots)
    {
        renderSlot (sl, c);

        if (! sl.sampledIn && ! sl.apparition)
        {
            sl.sampledIn = true;
            releaseParticles (sl, c, false);
        }
        if (sl.leaving && ! sl.sampledOut && ! sl.apparition)
        {
            sl.sampledOut = true;
            releaseParticles (sl, c, true);
        }
    }
}

void TransmissionEngine::renderSlot (Slot& sl, const TxContext& c)
{
    const auto& screen = c.screen;
    if (screen.getWidth() < 8.0f || screen.getHeight() < 8.0f)
        return;

    const float aspect = juce::jlimit (1.2f, 2.4f, screen.getWidth() / screen.getHeight());
    const int w = juce::jmax (64, juce::jmin (560, juce::roundToInt (screen.getWidth() * 0.6f)));
    const int h = juce::jmax (32, juce::roundToInt ((float) w / aspect));

    if (sl.image.getWidth() != w || sl.image.getHeight() != h)
        sl.image = juce::Image (juce::Image::ARGB, w, h, true);
    else
        sl.image.clear (sl.image.getBounds());

    const auto& spec = sceneLibrary()[(size_t) sl.scene];
    const auto& f = *c.audio;

    SceneParams p;
    p.t = sl.age;
    p.beat = beatNow;
    p.bass = f.bass;
    p.mid = f.mid;
    p.high = f.high;
    p.kick = f.kick;
    p.snare = f.snare;
    p.intensity = f.intensity;
    p.mystery = c.settings.mystery;
    p.seed = sl.seed;
    p.variant = spec.variant;
    p.ink = inkFromPalette();

    juce::Graphics ig (sl.image);
    spec.draw (ig, juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h), p);
}

juce::Rectangle<float> TransmissionEngine::destRect (const TxContext& c, const juce::Image& img) const
{
    const auto s = c.screen;
    const float aspect = (float) img.getWidth() / (float) juce::jmax (1, img.getHeight());
    float dw = s.getWidth(), dh = dw / aspect;
    if (dh < s.getHeight())
    {
        dh = s.getHeight();
        dw = dh * aspect;
    }
    return juce::Rectangle<float> (dw, dh).withCentre (s.getCentre());
}

void TransmissionEngine::releaseParticles (const Slot& sl, const TxContext& c, bool dissolve)
{
    if (c.particles == nullptr || ! sl.image.isValid())
        return;

    const auto dest = destRect (c, sl.image);
    const int iw = sl.image.getWidth(), ih = sl.image.getHeight();
    const int step = juce::jmax (4, iw / 44);
    const float density = c.settings.density;
    int spawned = 0;

    const juce::Image::BitmapData data (sl.image, juce::Image::BitmapData::readOnly);

    for (int y = step / 2; y < ih && spawned < 140; y += step)
    {
        for (int x = step / 2; x < iw && spawned < 140; x += step)
        {
            const auto col = data.getPixelColour (x, y);
            const float b = col.getBrightness();
            const float strength = (pal::lightScreen ? 1.0f - b : b) * col.getFloatAlpha();

            if (strength < 0.35f || rng.nextFloat() > 0.25f + 0.4f * density)
                continue;

            const juce::Point<float> pos (dest.getX() + (float) x / (float) iw * dest.getWidth(),
                                          dest.getY() + (float) y / (float) ih * dest.getHeight());
            if (! c.screen.contains (pos))
                continue;

            if (dissolve)
                c.particles->spawn (ParticleType::Pixel, pos, { (rng.nextFloat() - 0.5f) * 30.0f, -10.0f - rng.nextFloat() * 40.0f },
                                    1.2f + rng.nextFloat() * 1.4f, c.px * 0.7f, col.withAlpha (0.8f));
            else
                c.particles->spawn (ParticleType::Spark, pos, { (rng.nextFloat() - 0.5f) * 12.0f, (rng.nextFloat() - 0.5f) * 12.0f },
                                    0.5f + rng.nextFloat() * 0.5f, c.px * 0.5f, col.withAlpha (0.9f));
            ++spawned;
        }
    }
}

// =================================================================================================
//  Compositing: the image lives inside the signal
// =================================================================================================
void TransmissionEngine::drawStrips (juce::Graphics& g, const Slot& sl, juce::Rectangle<float> dest, float alpha,
                                     float amplitude, float time, bool materialise) const
{
    if (alpha <= 0.002f)
        return;

    const auto& img = sl.image;
    const int iw = img.getWidth(), ih = img.getHeight();
    const int strips = 36;
    const int frame = (int) (time * 24.0f);
    const float a = sl.alpha();

    g.setColour (juce::Colours::black);
    g.setOpacity (juce::jlimit (0.0f, 1.0f, alpha));

    for (int i = 0; i < strips; ++i)
    {
        // materialising: rows flicker in, as if the picture is struggling to come through
        if (materialise && a < 0.999f && kit::hash (sl.seed + (juce::uint32) frame, i) > a * 1.15f)
            continue;

        const int sy0 = i * ih / strips, sy1 = (i + 1) * ih / strips;
        const int dy0 = juce::roundToInt (dest.getY() + dest.getHeight() * (float) i / (float) strips);
        const int dy1 = juce::roundToInt (dest.getY() + dest.getHeight() * (float) (i + 1) / (float) strips);
        if (sy1 <= sy0 || dy1 <= dy0)
            continue;

        const float wobble = (kit::noise1 (sl.seed + 31u, (float) i * 0.7f + time * 3.0f) - 0.5f) * amplitude;
        const float tearRoll = kit::hash (sl.seed + 7u + (juce::uint32) frame, i);
        const float tear = tearRoll > 0.97f ? (tearRoll - 0.985f) * amplitude * 60.0f : 0.0f;

        g.drawImage (img, juce::roundToInt (dest.getX() + wobble + tear), dy0, juce::roundToInt (dest.getWidth()), dy1 - dy0,
                     0, sy0, iw, sy1 - sy0);
    }
}

void TransmissionEngine::draw (juce::Graphics& g, const TxContext& c) const
{
    if (slots.empty() || c.scope == nullptr || c.audio == nullptr)
        return;

    const auto& f = *c.audio;
    const auto& s = c.settings;
    const float vis = s.visibility;
    const float margin = c.px * (3.0f + 10.0f * s.density) + c.screen.getHeight() * 0.03f * s.density;
    const auto band = c.scope->signalBand (margin);

    for (const auto& sl : slots)
    {
        if (! sl.image.isValid())
            continue;

        const float a = sl.alpha();
        if (a <= 0.002f)
            continue;

        const auto dest = destRect (c, sl.image);
        const float amp = c.px * (1.0f + 3.0f * s.mystery + 8.0f * f.high + 10.0f * f.intensity + 12.0f * f.snare)
                          * (sl.apparition ? 2.0f : 1.0f) * (1.5f - 0.5f * a);

        // 1. ghost over the whole screen (not for apparitions - they only exist inside the signal)
        if (! sl.apparition)
            drawStrips (g, sl, dest, vis * a * (0.18f + 0.22f * s.density), amp * 0.6f, c.time, true);

        // 2. strong reveal inside the waveform's envelope
        {
            juce::Graphics::ScopedSaveState state (g);
            g.reduceClipRegion (band);
            const float reveal = vis * a * (0.6f + 0.35f * s.density) * (sl.apparition ? 0.6f + 0.6f * s.mystery : 1.0f);
            drawStrips (g, sl, dest, juce::jmin (1.0f, reveal), amp, c.time + 0.37f, true);

            // 3. chromatic ghosts when the music is intense
            const float chroma = vis * a * f.intensity * (0.06f + 0.1f * s.mystery);
            if (chroma > 0.004f)
            {
                const float off = c.px * (1.0f + 3.0f * f.intensity);
                const auto& img = sl.image;
                g.setColour (pal::cyan.withAlpha (chroma));
                g.drawImage (img, juce::roundToInt (dest.getX() + off), juce::roundToInt (dest.getY()), juce::roundToInt (dest.getWidth()),
                             juce::roundToInt (dest.getHeight()), 0, 0, img.getWidth(), img.getHeight(), true);
                g.setColour (pal::crimson.withAlpha (chroma));
                g.drawImage (img, juce::roundToInt (dest.getX() - off), juce::roundToInt (dest.getY()), juce::roundToInt (dest.getWidth()),
                             juce::roundToInt (dest.getHeight()), 0, 0, img.getWidth(), img.getHeight(), true);
            }
        }
    }

    // a slow scanline roll through the transmission
    const float roll = std::fmod (c.time * 0.12f, 1.2f) - 0.1f;
    const float ry = c.screen.getY() + c.screen.getHeight() * roll;
    juce::ColourGradient rollGrad (pal::moon.withAlpha (0.0f), 0.0f, ry - 20.0f, pal::moon.withAlpha (0.0f), 0.0f, ry + 20.0f, false);
    rollGrad.addColour (0.5, pal::moon.withAlpha (0.035f * vis));
    g.setGradientFill (rollGrad);
    g.fillRect (c.screen.getX(), ry - 20.0f, c.screen.getWidth(), 40.0f);
}
}

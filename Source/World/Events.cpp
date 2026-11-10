// =================================================================================================
//  TOKEN - the living world's scenes.
//  Every scene is original artwork drawn with simple pixel-art and vector primitives.
//  To add a scene: subclass WorldEvent, then register it in createEventLibrary() at the bottom.
// =================================================================================================
#include "WorldTypes.h"
#include "PixelArt.h"
#include "../Palette.h"
#include <cmath>

namespace token
{
namespace
{
constexpr float pi    = juce::MathConstants<float>::pi;
constexpr float twoPi = juce::MathConstants<float>::twoPi;

template <typename T>
std::unique_ptr<WorldEvent> makeEvent (const WorldContext& c) { return std::make_unique<T> (c); }

// =================================================================================================
//  1. TINY AEROPLANE - flies just above the waveform peaks, contrail, blinking navigation lights
// =================================================================================================
const char* const planeSprite[] = {
    "tt................",
    "ttt...............",
    "tttt.......ww.....",
    ".bbbbbbbbbbbbbbbb.",
    ".bbgbgbgbgbgbgbbbb",
    "..bbbbbbbbbbbbbbb.",
    ".......wwwww......",
};

juce::Colour planePalette (char ch)
{
    switch (ch)
    {
        case 'b': return juce::Colour (0xffcfdcec);
        case 'w': return juce::Colour (0xff8fa3bd);
        case 't': return juce::Colour (0xffa9bad0);
        case 'g': return juce::Colour (0xffffc878);
        default:  return juce::Colours::transparentBlack;
    }
}

class Aeroplane final : public WorldEvent
{
public:
    explicit Aeroplane (const WorldContext& c)
    {
        layer = Layer::Front;
        dir = c.rng->nextBool() ? 1.0f : -1.0f;
        speed = c.screen.getWidth() / c.rand (9.0f, 14.0f);
        const float w = 18.0f * c.px;
        x = dir > 0.0f ? c.screen.getX() - w : c.screen.getRight() + w;
        y = c.screen.getY() + c.screen.getHeight() * c.rand (0.15f, 0.35f);
        duration = (c.screen.getWidth() + 2.0f * w) / speed;
        fadeIn = fadeOut = 0.5f;
    }

    void update (const WorldContext& c) override
    {
        x += dir * speed * c.dt;

        const float top = c.screen.getY() + 5.0f * c.px;
        const float bottom = c.screen.getBottom() - 14.0f * c.px;
        const float target = juce::jlimit (top, juce::jmax (top, bottom),
                                           c.scope->envelopeTopAt (x) - 10.0f * c.px - c.kick() * 4.0f * c.px);
        y = smoothTo (y, target, c.dt, 0.7f);

        trailTimer += c.dt;
        if (trailTimer > 0.05f)
        {
            trailTimer = 0.0f;
            const float tailX = x - dir * 8.5f * c.px;
            c.particles->spawn (ParticleType::Dust, { tailX, y - 2.0f * c.px }, { -dir * 6.0f, 0.0f }, 1.6f,
                                c.px * 0.7f, pal::moon.withAlpha (0.2f + 0.25f * c.high()));
        }
    }

    void draw (juce::Graphics& g, const WorldContext& c, float alpha) const override
    {
        const float px = c.px;
        const float bob = std::sin (age * 2.1f) * 0.5f * px;
        const float sx = x - 9.0f * px;
        const float sy = y + bob - 3.5f * px;

        // cabin glow reacts to high frequencies
        g.setColour (pal::amber.withAlpha (alpha * 0.10f * (0.4f + c.high())));
        g.fillRoundedRectangle (sx + 1.0f * px, sy + 2.5f * px, 16.0f * px, 4.0f * px, 2.0f * px);

        pixel::drawSprite (g, planeSprite, 7, sx, sy, px, planePalette, alpha, dir < 0.0f);

        // red beacon on the tail
        const float tailX = dir > 0.0f ? sx + 0.5f * px : sx + 17.5f * px;
        const float beacon = 0.5f + 0.5f * std::sin (age * 6.0f);
        g.setColour (pal::crimson.withAlpha (alpha * (0.35f + 0.65f * beacon)));
        g.fillEllipse (tailX - px, sy - px, 2.0f * px, 2.0f * px);

        // white strobe under the wing
        if (std::fmod (age, 1.3f) < 0.09f)
        {
            const float wx = sx + 9.5f * px, wy = sy + 6.5f * px;
            g.setColour (juce::Colours::white.withAlpha (alpha * 0.25f));
            g.fillEllipse (wx - 3.0f * px, wy - 3.0f * px, 6.0f * px, 6.0f * px);
            g.setColour (juce::Colours::white.withAlpha (alpha));
            g.fillEllipse (wx - 0.6f * px, wy - 0.6f * px, 1.2f * px, 1.2f * px);
        }
    }

private:
    float dir = 1.0f, speed = 100.0f, x = 0.0f, y = 0.0f, trailTimer = 0.0f;
};

// =================================================================================================
//  2. RAIN CLOUD - procedurally shaped pixel cloud, swells with bass, rain follows the highs,
//                  lightning on kicks during CHAOS
// =================================================================================================
class RainCloud final : public WorldEvent
{
public:
    explicit RainCloud (const WorldContext& c)
    {
        layer = Layer::Mid;
        gw = 20 + c.rng->nextInt (12);
        gh = 10;
        mask.assign ((size_t) (gw * gh), 0);

        struct Blob { float x, y, r; };
        std::vector<Blob> blobs;
        const int numBlobs = 4 + c.rng->nextInt (3);

        for (int i = 0; i < numBlobs; ++i)
            blobs.push_back ({ 3.0f + c.rng->nextFloat() * (float) (gw - 6),
                               (float) gh - 3.5f - c.rng->nextFloat() * 2.0f,
                               2.5f + c.rng->nextFloat() * 2.2f });

        for (int r = 0; r < gh; ++r)
        {
            for (int col = 0; col < gw; ++col)
            {
                bool inside = (r >= gh - 3 && col >= 2 && col < gw - 2);
                const float fx = (float) col + 0.5f, fy = (float) r + 0.5f;

                for (const auto& b : blobs)
                {
                    const float dx = fx - b.x, dy = (fy - b.y) * 1.25f;
                    if (dx * dx + dy * dy <= b.r * b.r)
                        inside = true;
                }

                mask[(size_t) (r * gw + col)] = (juce::uint8) (inside ? 1 : 0);
            }
        }

        cx = c.screen.getX() + c.screen.getWidth() * c.rand (0.15f, 0.85f);
        cy = c.screen.getY() + c.screen.getHeight() * c.rand (0.12f, 0.30f);
        drift = c.screen.getWidth() / c.rand (50.0f, 80.0f) * (c.rng->nextBool() ? 1.0f : -1.0f);
        duration = c.rand (16.0f, 26.0f);
        fadeIn = 3.0f;
        fadeOut = 3.5f;
    }

    void update (const WorldContext& c) override
    {
        cx += drift * c.dt;
        flash *= std::exp (-c.dt / 0.12f);

        if (c.f().kickHit && c.chaosState() > 0.4f && c.rng->nextFloat() < 0.3f)
        {
            flash = 1.0f;
            boltOffset = c.rand (-1.0f, 1.0f);
        }

        const float scale = 1.0f + 0.14f * c.bass();
        const float w = (float) gw * c.px * scale;
        rainAcc += c.dt * (6.0f + 45.0f * c.high() + 30.0f * c.snare()) * envelope();

        while (rainAcc >= 1.0f)
        {
            rainAcc -= 1.0f;
            const float rx = cx - w * 0.42f + c.rng->nextFloat() * w * 0.84f;
            const float ry = cy + (float) gh * c.px * scale * 0.5f;
            const float bright = 0.35f + 0.5f * c.high();
            c.particles->spawn (ParticleType::Rain, { rx, ry },
                                { drift * 0.6f + c.rand (-6.0f, 6.0f), c.screen.getHeight() * c.rand (1.1f, 1.5f) },
                                2.0f, 1.0f, pal::turquoise.interpolatedWith (pal::moon, 0.5f).withAlpha (bright));
        }
    }

    void draw (juce::Graphics& g, const WorldContext& c, float alpha) const override
    {
        const float scale = 1.0f + 0.14f * c.bass();
        const float cpx = c.px * scale;
        const float ox = cx - (float) gw * cpx * 0.5f;
        const float oy = cy - (float) gh * cpx * 0.5f;
        const juce::Colour hi (0xff9aaccb), mid (0xff56657f), lo (0xff2c364b);

        auto cell = [this] (int r, int col)
        {
            return r >= 0 && r < gh && col >= 0 && col < gw && mask[(size_t) (r * gw + col)] != 0;
        };

        // soft body haze
        g.setColour (mid.withAlpha (0.08f * alpha));
        g.fillEllipse (ox - 2.0f * cpx, oy, (float) gw * cpx + 4.0f * cpx, (float) gh * cpx);

        for (int r = 0; r < gh; ++r)
        {
            const int y0 = juce::roundToInt (oy + (float) r * cpx);
            const int y1 = juce::roundToInt (oy + (float) (r + 1) * cpx);

            for (int col = 0; col < gw; ++col)
            {
                if (! cell (r, col))
                    continue;

                auto colour = ! cell (r - 1, col) ? hi : (! cell (r + 1, col) ? lo : mid);
                colour = colour.interpolatedWith (pal::moon, flash * 0.8f);

                const int x0 = juce::roundToInt (ox + (float) col * cpx);
                const int x1 = juce::roundToInt (ox + (float) (col + 1) * cpx);
                g.setColour (colour.withMultipliedAlpha (alpha));
                g.fillRect (x0, y0, x1 - x0, y1 - y0);
            }
        }

        if (flash > 0.3f)
        {
            juce::Path bolt;
            float bx = cx + boltOffset * (float) gw * cpx * 0.3f;
            float by = oy + (float) gh * cpx;
            bolt.startNewSubPath (bx, by);
            for (int i = 1; i <= 6; ++i)
            {
                bx += ((i % 2) != 0 ? 1.0f : -1.0f) * cpx * 1.5f;
                by += cpx * 3.0f;
                bolt.lineTo (bx, by);
            }
            pixel::drawGlowPath (g, bolt, pal::moon, flash * alpha, juce::jmax (1.0f, cpx * 0.4f));
        }
    }

private:
    std::vector<juce::uint8> mask;
    int gw = 24, gh = 10;
    float cx = 0.0f, cy = 0.0f, drift = 0.0f, rainAcc = 0.0f, flash = 0.0f, boltOffset = 0.0f;
};

// =================================================================================================
//  3. TWO PEOPLE ON A BENCH BENEATH A STREETLIGHT - silhouettes with warm rim light
// =================================================================================================
class BenchScene final : public WorldEvent
{
public:
    explicit BenchScene (const WorldContext& c)
    {
        layer = Layer::Back;
        duration = c.rand (20.0f, 28.0f);
        fadeIn = 3.5f;
        fadeOut = 3.5f;
        bx = c.screen.getX() + c.screen.getWidth() * c.rand (0.2f, 0.8f);
    }

    void update (const WorldContext& c) override
    {
        if (c.rng->nextFloat() < c.dt * 0.12f)
            flicker = 1.0f;
        flicker *= std::exp (-c.dt / 0.15f);

        // a few motes drifting through the lamplight
        moteAcc += c.dt * (0.8f + 2.0f * c.high()) * envelope();
        while (moteAcc >= 1.0f)
        {
            moteAcc -= 1.0f;
            const float ground = c.screen.getBottom() - 3.0f * c.px;
            c.particles->spawn (ParticleType::Dust,
                                { bx + c.rand (-12.0f, 8.0f) * c.px, ground - c.rand (4.0f, 22.0f) * c.px },
                                { c.rand (-4.0f, 4.0f), c.rand (-3.0f, 3.0f) }, c.rand (2.0f, 4.0f),
                                c.px * 0.6f, juce::Colour (0xffffd9a0).withAlpha (0.6f));
        }
    }

    void draw (juce::Graphics& g, const WorldContext& c, float alpha) const override
    {
        const float px = c.px;
        const float ground = c.screen.getBottom() - 3.0f * px;

        // rectangle in "units": x from bench origin, y measured upwards from the ground
        auto R = [&] (float ux, float uy, float uw, float uh, juce::Colour col)
        {
            const int x0 = juce::roundToInt (bx + ux * px);
            const int x1 = juce::roundToInt (bx + (ux + uw) * px);
            const int y1 = juce::roundToInt (ground - uy * px);
            const int y0 = juce::roundToInt (ground - (uy + uh) * px);
            g.setColour (col.withMultipliedAlpha (alpha));
            g.fillRect (x0, y0, juce::jmax (1, x1 - x0), juce::jmax (1, y1 - y0));
        };

        const float light = juce::jlimit (0.0f, 1.0f, (0.6f + 0.3f * c.f().mid + 0.08f * std::sin (age * 1.3f))
                                                          * (1.0f - 0.65f * flicker));
        const juce::Point<float> lamp (bx - 5.0f * px, ground - 22.0f * px);
        const juce::Colour warm (0xffffc98a);

        // light cone, halo and pool on the ground
        {
            juce::Path cone;
            cone.startNewSubPath (lamp.x - 1.5f * px, lamp.y);
            cone.lineTo (lamp.x + 1.5f * px, lamp.y);
            cone.lineTo (bx + 16.0f * px, ground);
            cone.lineTo (bx - 18.0f * px, ground);
            cone.closeSubPath();

            juce::ColourGradient coneGrad (warm.withAlpha (0.20f * light * alpha), lamp.x, lamp.y,
                                           warm.withAlpha (0.0f), lamp.x, ground + 2.0f * px, false);
            g.setGradientFill (coneGrad);
            g.fillPath (cone);

            juce::ColourGradient halo (warm.withAlpha (0.45f * light * alpha), lamp.x, lamp.y,
                                       warm.withAlpha (0.0f), lamp.x + 9.0f * px, lamp.y, true);
            g.setGradientFill (halo);
            g.fillEllipse (lamp.x - 9.0f * px, lamp.y - 9.0f * px, 18.0f * px, 18.0f * px);

            g.setColour (warm.withAlpha (0.10f * light * alpha));
            g.fillEllipse (bx - 16.0f * px, ground - 1.5f * px, 30.0f * px, 3.0f * px);
        }

        const juce::Colour pole (0xff1a1e28), sil (0xff0d1018), wood (0xff2a1f18);
        const auto rim = warm.withAlpha (0.55f * light);

        R (-22.0f, -1.0f, 44.0f, 1.0f, juce::Colour (0xff0b0f18));         // ground

        // streetlight
        R (-9.0f, 0.0f, 1.0f, 23.0f, pole);
        R (-9.0f, 23.0f, 5.0f, 1.0f, pole);
        R (-6.5f, 22.0f, 3.0f, 1.0f, pole);
        R (-6.0f, 21.5f, 2.0f, 0.5f, warm.withAlpha (0.6f + 0.4f * light));
        R (-8.4f, 0.0f, 0.4f, 21.0f, rim.withMultipliedAlpha (0.5f));

        // bench
        R (-1.0f, 0.0f, 1.0f, 4.0f, wood);
        R (12.0f, 0.0f, 1.0f, 4.0f, wood);
        R (-1.5f, 4.0f, 15.0f, 1.0f, wood);
        R (-1.5f, 5.0f, 15.0f, 0.35f, rim);
        R (-1.0f, 5.0f, 1.0f, 4.0f, wood);
        R (12.0f, 5.0f, 1.0f, 4.0f, wood);
        R (-1.5f, 7.5f, 15.0f, 0.8f, wood);
        R (-1.5f, 8.3f, 15.0f, 0.3f, rim);

        // person A - swinging one leg, violet hat
        const float swing = std::fmod (age * 0.9f, 2.0f) < 1.0f ? 0.0f : 0.5f;
        R (1.5f, 0.6f, 1.0f, 3.4f, sil);
        R (3.5f + swing, 0.6f, 1.0f, 3.4f, sil);
        R (1.0f, 5.0f, 4.0f, 5.0f, sil);
        R (1.0f, 5.0f, 0.5f, 5.0f, rim);
        R (2.0f, 10.0f, 2.0f, 2.0f, sil);
        R (2.0f, 11.5f, 2.0f, 0.7f, juce::Colour (0xff3c2f63));
        R (2.0f, 10.0f, 0.4f, 2.0f, rim);

        // person B - crimson scarf, occasionally leans in
        const float lean = std::sin (age * 0.33f) > 0.55f ? -0.7f : 0.0f;
        R (7.0f, 0.6f, 1.0f, 3.4f, sil);
        R (9.5f, 0.6f, 1.0f, 3.4f, sil);
        R (6.5f, 5.0f, 4.0f, 4.6f, sil);
        R (6.5f, 5.0f, 0.5f, 4.6f, rim);
        R (7.6f + lean, 9.6f, 2.0f, 2.0f, sil);
        R (7.2f, 9.2f, 2.6f, 0.6f, juce::Colour (0xff7a2a3a));
        R (7.6f + lean, 9.6f, 0.4f, 2.0f, rim);
    }

private:
    float bx = 0.0f, flicker = 0.0f, moteAcc = 0.0f;
};

// =================================================================================================
//  4. VIKING LONGSHIP WITH A GLOWING RUNE - sails on the waveform as if the sound were water
// =================================================================================================
const char* const shipSprite[] = {
    ".............m..............",
    ".......ssssssssssssss.......",
    ".......rrrrrrrrrrrrrr.......",
    ".......ssssssssssssss.......",
    ".......rrrrrrrrrrrrrr.......",
    ".......ssssssssssssss.......",
    ".......rrrrrrrrrrrrrr.....dd",
    "d............m...........dd.",
    "hd...........m..........hh..",
    ".hHgHgHgHgHgHgHgHgHgHgHgh...",
    "..hhhhhhhhhhhhhhhhhhhhhh....",
    "....hhhhhhhhhhhhhhhhhh......",
};

juce::Colour shipPalette (char ch)
{
    switch (ch)
    {
        case 's': return juce::Colour (0xffd9ccb0);
        case 'r': return juce::Colour (0xff9c2536);
        case 'm': return juce::Colour (0xff2a1e15);
        case 'h': return juce::Colour (0xff3a281c);
        case 'H': return juce::Colour (0xff5e4430);
        case 'g': return juce::Colour (0xffc9a45c);
        case 'd': return juce::Colour (0xff8a6a32);
        default:  return juce::Colours::transparentBlack;
    }
}

class Longship final : public WorldEvent
{
public:
    explicit Longship (const WorldContext& c)
    {
        layer = Layer::Front;
        const float shipW = 28.0f * c.px;
        x = c.screen.getX() - shipW;
        y = c.scope->waveYAt (c.screen.getX());
        speed = c.screen.getWidth() / c.rand (16.0f, 22.0f);
        duration = 120.0f;                    // ends when it leaves the screen
        fadeIn = 1.0f;
        fadeOut = 0.0f;
        static const int voyageRunes[] = { pixel::findRune ("raidho"), pixel::findRune ("tiwaz"),
                                           pixel::findRune ("algiz"), pixel::findRune ("ehwaz") };
        rune = voyageRunes[c.rng->nextInt (4)];
    }

    void update (const WorldContext& c) override
    {
        const float shipW = 28.0f * c.px;
        x += speed * (1.0f + 1.2f * c.bass()) * c.dt;
        oarPhase += c.dt * (2.0f + 5.0f * c.bass());

        const float span = shipW * 0.3f;
        const float yl = c.scope->waveYAt (x - span), ym = c.scope->waveYAt (x), yr = c.scope->waveYAt (x + span);
        y = smoothTo (y, (yl + ym + yr) / 3.0f, c.dt, 0.25f);
        tilt = smoothTo (tilt, juce::jlimit (-0.3f, 0.3f, std::atan ((yr - yl) / (2.0f * span)) * 0.6f), c.dt, 0.3f);

        runeGlow = juce::jmax (runeGlow * std::exp (-c.dt / 0.4f), 0.25f + 0.75f * c.bass(), c.kick());

        wakeTimer += c.dt;
        if (wakeTimer > 0.06f)
        {
            wakeTimer = 0.0f;
            c.particles->spawn (ParticleType::Spark, { x - 13.0f * c.px, y }, { -20.0f, c.rand (-6.0f, 2.0f) },
                                0.8f, c.px * 0.5f, pal::cyan.interpolatedWith (pal::moon, 0.5f).withAlpha (0.5f));
        }

        if (x - shipW > c.screen.getRight())
            age = duration;
    }

    void draw (juce::Graphics& g, const WorldContext& c, float alpha) const override
    {
        const float px = c.px;

        {
            juce::Graphics::ScopedSaveState state (g);
            g.addTransform (juce::AffineTransform::rotation (tilt, x, y));

            for (int i = 0; i < 6; ++i)
            {
                const float ox = x + (-9.0f + 3.5f * (float) i) * px;
                const float sweep = std::sin (oarPhase + (float) i * 0.5f) * 2.0f * px;
                g.setColour (juce::Colour (0xff3b2a1e).withAlpha (alpha));
                g.drawLine (ox, y - 1.0f * px, ox - 2.0f * px + sweep, y + 3.5f * px, juce::jmax (1.0f, px * 0.5f));
            }

            pixel::drawSprite (g, shipSprite, 12, x - 14.0f * px, y - 10.0f * px, px, shipPalette, alpha);
        }

        const auto box = juce::Rectangle<float> (6.0f * px, 9.0f * px).withCentre ({ x, y - 19.0f * px });
        const auto colour = pal::gold.interpolatedWith (pal::cyan, 0.35f);
        juce::ColourGradient halo (colour.withAlpha (0.18f * runeGlow * alpha), box.getCentreX(), box.getCentreY(),
                                   colour.withAlpha (0.0f), box.getCentreX() + box.getHeight(), box.getCentreY(), true);
        g.setGradientFill (halo);
        g.fillEllipse (box.expanded (box.getHeight() * 0.6f));
        pixel::drawRune (g, rune, box, colour, alpha * (0.3f + 0.7f * runeGlow), juce::jmax (1.2f, px * 0.45f));
    }

private:
    float x = 0.0f, y = 0.0f, tilt = 0.0f, speed = 60.0f, oarPhase = 0.0f, runeGlow = 0.0f, wakeTimer = 0.0f;
    int rune = 0;
};

// =================================================================================================
//  5. MYSTICAL EYE ABOVE A STEPPED PYRAMID - pyramid edges glow with bass, the eye opens in CHAOS
// =================================================================================================
class EyePyramid final : public WorldEvent
{
public:
    explicit EyePyramid (const WorldContext& c)
    {
        layer = Layer::Back;
        duration = c.rand (11.0f, 16.0f);
        fadeIn = 2.5f;
        fadeOut = 2.5f;
        height = c.screen.getHeight() * (c.strip ? 0.62f : 0.42f);
        baseW = height * 1.6f;
        cx = c.screen.getX() + c.screen.getWidth() * c.rand (0.25f, 0.75f);
        nextBlink = c.rand (2.5f, 5.0f);
    }

    void update (const WorldContext& c) override
    {
        const float openTarget = age < 1.5f ? 0.0f : 0.55f + 0.45f * c.chaosState();
        open = smoothTo (open, openTarget, c.dt, 0.6f);

        nextBlink -= c.dt;
        if (nextBlink <= 0.0f) { blinkT = 0.0f; nextBlink = c.rand (2.5f, 5.0f); }
        blinkT += c.dt;

        if (c.f().snareHit)
        {
            pupilTarget = { c.rand (-1.0f, 1.0f), c.rand (-0.4f, 0.4f) };
        }
        pupil.x = smoothTo (pupil.x, pupilTarget.x + (c.high() - c.bass()) * 0.4f, c.dt, 0.15f);
        pupil.y = smoothTo (pupil.y, pupilTarget.y, c.dt, 0.15f);

        rays = juce::jmax (rays * std::exp (-c.dt / 0.3f), c.kick() * c.chaosState());
    }

    void draw (juce::Graphics& g, const WorldContext& c, float alpha) const override
    {
        const auto& s = c.screen;
        const float rise = smoothstep (age / 2.5f) * smoothstep ((duration - age) / 2.5f);
        const float baseY = s.getBottom() + (1.0f - rise) * height * 0.9f;
        const float apexY = baseY - height;
        const float bassGlow = 0.15f + 0.85f * c.bass();

        // stepped pyramid
        const int steps = juce::jmax (6, (int) (height / (c.px * 2.0f)));
        const float stepH = height / (float) steps;

        for (int i = 0; i < steps; ++i)
        {
            const float frac = (float) i / (float) steps;
            const float w = baseW * (1.0f - frac);
            const float yTop = baseY - (float) (i + 1) * stepH;
            const auto stone = juce::Colour (0xff17120c).interpolatedWith (juce::Colour (0xff3a2c1a), frac * 0.6f);

            g.setColour (stone.withAlpha (alpha));
            g.fillRect (cx - w * 0.5f, yTop, w, stepH + 0.5f);
            g.setColour (juce::Colours::black.withAlpha (0.35f * alpha));
            g.fillRect (cx, yTop, w * 0.5f, stepH + 0.5f);                     // shaded right face
            g.setColour (pal::gold.withAlpha (juce::jlimit (0.0f, 1.0f, (0.1f + 0.5f * bassGlow) * alpha * (1.0f - frac * 0.5f))));
            g.fillRect (cx - w * 0.5f, yTop, w, juce::jmax (1.0f, c.px * 0.35f)); // lit step edge
        }

        juce::Path outline;
        outline.startNewSubPath (cx - baseW * 0.5f, baseY);
        outline.lineTo (cx, apexY);
        outline.lineTo (cx + baseW * 0.5f, baseY);
        pixel::drawGlowPath (g, outline, pal::gold, alpha * 0.6f * bassGlow, 1.0f);

        // capstone
        g.setColour (pal::goldBright.withAlpha (alpha * (0.4f + 0.6f * bassGlow)));
        g.fillRect (cx - stepH, apexY - stepH * 0.5f, stepH * 2.0f, stepH);

        // the eye
        const float eyeW = baseW * 0.42f;
        const float ey = apexY - height * 0.3f;
        const float blink = blinkT < 0.18f ? std::sin (pi * blinkT / 0.18f) : 0.0f;
        const float openEff = juce::jmax (0.0f, open * (1.0f - blink));
        const float eyeH = eyeW * 0.42f * openEff;

        juce::ColourGradient halo (pal::gold.withAlpha (0.14f * open * alpha), cx, ey,
                                   pal::gold.withAlpha (0.0f), cx + eyeW, ey, true);
        g.setGradientFill (halo);
        g.fillEllipse (cx - eyeW, ey - eyeW, eyeW * 2.0f, eyeW * 2.0f);

        // CHAOS rays
        const float chaosW = c.chaosState();
        if (chaosW > 0.05f)
        {
            for (int i = 0; i < 16; ++i)
            {
                const float a = (float) i / 16.0f * twoPi + c.time * 0.15f;
                const float r0 = eyeW * 0.6f;
                const float r1 = eyeW * (0.9f + 0.6f * rays + 0.15f * std::sin (c.time * 2.0f + (float) i));
                g.setColour (pal::gold.withAlpha (0.22f * chaosW * alpha));
                g.drawLine (cx + std::cos (a) * r0, ey + std::sin (a) * r0, cx + std::cos (a) * r1, ey + std::sin (a) * r1, 1.0f);
            }
        }

        juce::Path almond;
        almond.startNewSubPath (cx - eyeW * 0.5f, ey);
        almond.quadraticTo (cx, ey - eyeH, cx + eyeW * 0.5f, ey);
        almond.quadraticTo (cx, ey + eyeH, cx - eyeW * 0.5f, ey);
        almond.closeSubPath();

        if (eyeH > 0.6f)
        {
            g.setColour (juce::Colour (0xff0d0f18).withAlpha (alpha));
            g.fillPath (almond);

            juce::Graphics::ScopedSaveState state (g);
            g.reduceClipRegion (almond);

            const float ir = eyeW * 0.2f;
            const float ix = cx + pupil.x * eyeW * 0.15f, iy = ey + pupil.y * eyeH * 0.3f;
            juce::ColourGradient iris (pal::cyan.withAlpha (alpha), ix, iy,
                                       juce::Colour (0xff04161c).withAlpha (alpha), ix + ir, iy, true);
            iris.addColour (0.55, pal::turquoise.withMultipliedAlpha (alpha * 0.8f));
            g.setGradientFill (iris);
            g.fillEllipse (ix - ir, iy - ir, ir * 2.0f, ir * 2.0f);

            const float pr = ir * (0.42f - 0.12f * c.kick());
            g.setColour (juce::Colours::black.withAlpha (alpha));
            g.fillEllipse (ix - pr, iy - pr, pr * 2.0f, pr * 2.0f);
            g.setColour (juce::Colours::white.withAlpha (0.8f * alpha));
            g.fillEllipse (ix - ir * 0.45f, iy - ir * 0.5f, ir * 0.22f, ir * 0.22f);
        }

        pixel::drawGlowPath (g, almond, pal::goldBright, alpha * (0.5f + 0.5f * open), 1.0f);
    }

private:
    float cx = 0.0f, baseW = 100.0f, height = 60.0f;
    float open = 0.0f, nextBlink = 3.0f, blinkT = 1.0f, rays = 0.0f;
    juce::Point<float> pupil, pupilTarget;
};

// =================================================================================================
//  COMMON: rune emerging from electronic noise, dissolving into particles
// =================================================================================================
class RuneFlash final : public WorldEvent
{
public:
    explicit RuneFlash (const WorldContext& c)
    {
        layer = Layer::Mid;
        duration = c.rand (4.0f, 7.0f);
        fadeIn = 1.2f;
        fadeOut = 1.2f;
        rune = c.rng->nextInt (pixel::numRunes());
        size = juce::jmin (c.screen.getHeight() * 0.32f, 16.0f * c.px);
        centre = { c.screen.getX() + c.screen.getWidth() * c.rand (0.1f, 0.9f),
                   c.scope->centreY() + c.screen.getHeight() * c.rand (-0.28f, 0.28f) };
        colour = c.theme[1] > c.theme[2] ? pal::gold : pal::cyan.interpolatedWith (pal::turquoise, 0.4f);
    }

    void update (const WorldContext& c) override
    {
        glow = juce::jmax (smoothTo (glow, 0.35f + 0.65f * c.bass(), c.dt, 0.1f, 0.5f), c.kick());

        if (! dissolved && age > duration - fadeOut)
        {
            dissolved = true;
            const auto path = pixel::runePath (rune, box());
            const float len = path.getLength();

            for (int i = 0; i < 26; ++i)
            {
                const auto p = path.getPointAlongPath (len * c.rng->nextFloat());
                c.particles->spawn (ParticleType::Spark, p, { c.rand (-25.0f, 25.0f), c.rand (-40.0f, 5.0f) },
                                    c.rand (0.8f, 1.6f), c.px * 0.6f, colour);
            }
        }
    }

    void draw (juce::Graphics& g, const WorldContext& c, float alpha) const override
    {
        const auto b = box();
        const float emerge = juce::jlimit (0.0f, 1.0f, age / fadeIn);

        // electronic noise that condenses into the glyph
        const int specks = (int) ((1.0f - emerge) * 40.0f) + 6;
        for (int i = 0; i < specks; ++i)
        {
            const float sx = b.getX() + c.rand (-0.5f, 1.5f) * b.getWidth();
            const float sy = b.getY() + c.rand (-0.2f, 1.2f) * b.getHeight();
            g.setColour (colour.withAlpha ((0.08f + 0.4f * (1.0f - emerge)) * alpha));
            g.fillRect (std::round (sx), std::round (sy), c.px * 0.6f, c.px * 0.6f);
        }

        pixel::drawRune (g, rune, b, colour, alpha * (0.35f + 0.65f * glow), juce::jmax (1.4f, size * 0.06f));
    }

private:
    juce::Rectangle<float> box() const { return juce::Rectangle<float> (size * 0.62f, size).withCentre (centre); }

    int rune = 0;
    float size = 30.0f, glow = 0.0f;
    bool dissolved = false;
    juce::Point<float> centre;
    juce::Colour colour;
};

// =================================================================================================
//  COMMON: shooting star
// =================================================================================================
class ShootingStar final : public WorldEvent
{
public:
    explicit ShootingStar (const WorldContext& c)
    {
        layer = Layer::Back;
        duration = c.rand (0.7f, 1.3f);
        fadeIn = 0.05f;
        fadeOut = 0.45f;
        const float dir = c.rng->nextBool() ? 1.0f : -1.0f;
        start = { c.screen.getX() + c.screen.getWidth() * c.rand (0.1f, 0.9f),
                  c.screen.getY() + c.screen.getHeight() * c.rand (0.03f, 0.35f) };
        vel = { dir * c.screen.getWidth() * c.rand (0.35f, 0.6f), c.screen.getHeight() * c.rand (0.15f, 0.35f) };
    }

    void draw (juce::Graphics& g, const WorldContext& c, float alpha) const override
    {
        const auto head = start + vel * age;
        const auto tail = head - vel * 0.2f;
        juce::ColourGradient grad (pal::moon.withAlpha (alpha), head.x, head.y, pal::moon.withAlpha (0.0f), tail.x, tail.y, false);
        g.setGradientFill (grad);
        g.drawLine ({ tail, head }, juce::jmax (1.0f, c.px * 0.5f));
        g.setColour (pal::moon.withAlpha (0.25f * alpha));
        g.fillEllipse (head.x - c.px * 1.5f, head.y - c.px * 1.5f, c.px * 3.0f, c.px * 3.0f);
    }

private:
    juce::Point<float> start, vel;
};

// =================================================================================================
//  UNCOMMON: ravens crossing during transitions
// =================================================================================================
const char* const ravenUp[] = {
    "w.......w",
    ".ww...ww.",
    "...bbb...",
    "....b....",
};

const char* const ravenDown[] = {
    "....b....",
    "..wbbbw..",
    ".w.....w.",
    "w.......w",
};

juce::Colour ravenPalette (char ch)
{
    switch (ch)
    {
        case 'b': return juce::Colour (0xff2a2c3a);
        case 'w': return juce::Colour (0xff5d6380);
        default:  return juce::Colours::transparentBlack;
    }
}

class Ravens final : public WorldEvent
{
public:
    explicit Ravens (const WorldContext& c)
    {
        layer = Layer::Front;
        dir = c.rng->nextBool() ? 1.0f : -1.0f;
        speed = c.screen.getWidth() / c.rand (7.0f, 10.0f);
        const int n = 3 + c.rng->nextInt (4);
        const float baseY = c.screen.getY() + c.screen.getHeight() * c.rand (0.12f, 0.35f);
        const float startX = dir > 0.0f ? c.screen.getX() - 12.0f * c.px : c.screen.getRight() + 12.0f * c.px;

        for (int i = 0; i < n; ++i)
            birds.push_back ({ startX - dir * (float) i * c.rand (6.0f, 11.0f) * c.px,
                               baseY + c.rand (-6.0f, 6.0f) * c.px,
                               c.rand (0.0f, 1.0f), c.rand (0.9f, 1.1f) });

        duration = (c.screen.getWidth() + 80.0f * c.px) / speed + 1.0f;
        fadeIn = 0.3f;
        fadeOut = 0.8f;
    }

    void update (const WorldContext& c) override
    {
        for (auto& b : birds)
        {
            b.x += dir * speed * b.speed * (1.0f + 0.3f * c.bass()) * c.dt;
            b.y += std::sin (age * 1.7f + b.phase * twoPi) * c.px * 2.0f * c.dt;
            b.phase += c.dt * (2.5f + 3.0f * c.high());
        }
    }

    void draw (juce::Graphics& g, const WorldContext& c, float alpha) const override
    {
        for (const auto& b : birds)
        {
            const bool up = std::fmod (b.phase, 1.0f) < 0.5f;
            pixel::drawSprite (g, up ? ravenUp : ravenDown, 4, b.x - 4.5f * c.px, b.y - 2.0f * c.px, c.px, ravenPalette, alpha, dir < 0.0f);
        }
    }

private:
    struct Bird { float x, y, phase, speed; };
    std::vector<Bird> birds;
    float dir = 1.0f, speed = 100.0f;
};

// =================================================================================================
//  VERY RARE: a whale rising briefly from a sea of particles
// =================================================================================================
class Whale final : public WorldEvent
{
public:
    explicit Whale (const WorldContext& c)
    {
        layer = Layer::Back;
        duration = 13.0f;
        fadeIn = 2.0f;
        fadeOut = 3.0f;
        length = c.screen.getHeight() * (c.strip ? 1.1f : 0.6f);
        cx = c.screen.getX() + c.screen.getWidth() * c.rand (0.3f, 0.7f);
        dir = c.rng->nextBool() ? 1.0f : -1.0f;
    }

    void update (const WorldContext& c) override
    {
        const float t = age / duration;
        if (! spouted && t > 0.45f)
        {
            spouted = true;
            const auto p = blowhole (c);
            for (int i = 0; i < 45; ++i)
                c.particles->spawn (ParticleType::Spark, p, { c.rand (-30.0f, 30.0f), -c.rand (60.0f, 160.0f) },
                                    c.rand (0.8f, 1.8f), c.px * 0.6f, pal::moon.interpolatedWith (pal::cyan, 0.4f));
        }
    }

    void draw (juce::Graphics& g, const WorldContext& c, float alpha) const override
    {
        const float L = length;
        juce::Path p;
        p.startNewSubPath (L * 0.5f, 0.0f);
        p.cubicTo (L * 0.45f, -L * 0.14f, L * 0.05f, -L * 0.17f, -L * 0.30f, -L * 0.07f);
        p.cubicTo (-L * 0.40f, -L * 0.04f, -L * 0.45f, -L * 0.02f, -L * 0.50f, -L * 0.09f);
        p.lineTo (-L * 0.56f, -L * 0.12f);
        p.lineTo (-L * 0.53f, 0.0f);
        p.lineTo (-L * 0.58f, L * 0.07f);
        p.lineTo (-L * 0.48f, L * 0.02f);
        p.cubicTo (-L * 0.30f, L * 0.06f, L * 0.10f, L * 0.12f, L * 0.5f, 0.0f);
        p.closeSubPath();

        p.applyTransform (transform (c));

        const auto bounds = p.getBounds();
        juce::ColourGradient body (juce::Colour (0xff0d2a48).withAlpha (alpha), 0.0f, bounds.getY(),
                                   juce::Colour (0xff040b16).withAlpha (alpha), 0.0f, bounds.getBottom(), false);
        g.setGradientFill (body);
        g.fillPath (p);
        pixel::drawGlowPath (g, p, pal::turquoise, alpha * (0.25f + 0.5f * c.bass()), 1.0f);
    }

private:
    juce::AffineTransform transform (const WorldContext& c) const
    {
        const float t = age / duration;
        const float rise = std::sin (pi * t);
        const float y = c.screen.getBottom() + length * 0.2f - rise * length * 0.45f;
        const float x = cx + dir * (t - 0.5f) * length * 0.6f;
        const float angle = dir * (0.5f - t) * 0.5f;
        return juce::AffineTransform::scale (dir, 1.0f).rotated (-angle).translated (x, y);
    }

    juce::Point<float> blowhole (const WorldContext& c) const
    {
        juce::Point<float> p (length * 0.2f, -length * 0.16f);
        p.applyTransform (transform (c));
        return p;
    }

    float length = 100.0f, cx = 0.0f, dir = 1.0f;
    bool spouted = false;
};

// =================================================================================================
//  VERY RARE: geometric portal
// =================================================================================================
class Portal final : public WorldEvent
{
public:
    explicit Portal (const WorldContext& c)
    {
        layer = Layer::Back;
        duration = c.rand (9.0f, 13.0f);
        fadeIn = 2.5f;
        fadeOut = 2.5f;
        radius = c.screen.getHeight() * (c.strip ? 0.38f : 0.26f);
        centre = { c.screen.getX() + c.screen.getWidth() * c.rand (0.25f, 0.75f),
                   c.screen.getY() + c.screen.getHeight() * c.rand (0.35f, 0.55f) };
    }

    void update (const WorldContext& c) override
    {
        spin += c.dt * (0.3f + 1.5f * c.f().intensity);
        acc += c.dt * 30.0f * envelope();

        while (acc >= 1.0f)
        {
            acc -= 1.0f;
            const float a = c.rand (0.0f, twoPi);
            const juce::Point<float> p (centre.x + std::cos (a) * radius * 1.3f, centre.y + std::sin (a) * radius * 1.3f);
            const auto toCentre = (centre - p) * 0.9f;
            const juce::Point<float> tangent (-std::sin (a) * radius, std::cos (a) * radius);
            c.particles->spawn (ParticleType::Spark, p, toCentre + tangent * 0.6f, 1.0f, c.px * 0.5f,
                                c.rng->nextBool() ? pal::violet.brighter (0.6f) : pal::gold);
        }
    }

    void draw (juce::Graphics& g, const WorldContext& c, float alpha) const override
    {
        const float r = radius * (0.3f + 0.7f * alpha);
        const float pulse = 1.0f + 0.08f * c.kick();

        juce::ColourGradient core (juce::Colours::black.withAlpha (alpha), centre.x, centre.y,
                                   pal::violet.withAlpha (0.0f), centre.x + r * 1.2f, centre.y, true);
        core.addColour (0.75, pal::violet.withAlpha (0.35f * alpha));
        g.setGradientFill (core);
        g.fillEllipse (centre.x - r * 1.2f, centre.y - r * 1.2f, r * 2.4f, r * 2.4f);

        juce::Path ring;
        ring.addEllipse (centre.x - r * pulse, centre.y - r * pulse, 2.0f * r * pulse, 2.0f * r * pulse);
        pixel::drawGlowPath (g, ring, pal::cyan, alpha * 0.8f, 1.0f);

        auto polygon = [&] (int sides, float rad, float rot)
        {
            juce::Path p;
            for (int i = 0; i <= sides; ++i)
            {
                const float a = rot + (float) i / (float) sides * twoPi;
                const juce::Point<float> v (centre.x + std::sin (a) * rad, centre.y - std::cos (a) * rad);
                if (i == 0) p.startNewSubPath (v); else p.lineTo (v);
            }
            return p;
        };

        pixel::drawGlowPath (g, polygon (6, r * 0.82f, spin), pal::gold, alpha * 0.7f, 1.0f);
        pixel::drawGlowPath (g, polygon (3, r * 0.7f, -spin * 0.7f), pal::gold, alpha * 0.6f, 1.0f);
        pixel::drawGlowPath (g, polygon (3, r * 0.7f, -spin * 0.7f + pi), pal::gold, alpha * 0.6f, 1.0f);

        for (int i = 0; i < 24; ++i)
        {
            const float a = spin * 0.3f + (float) i / 24.0f * twoPi;
            const float r0 = r * 1.08f, r1 = r * (i % 6 == 0 ? 1.22f : 1.14f);
            g.setColour (pal::gold.withAlpha (0.5f * alpha));
            g.drawLine (centre.x + std::sin (a) * r0, centre.y - std::cos (a) * r0,
                        centre.x + std::sin (a) * r1, centre.y - std::cos (a) * r1, 1.0f);
        }
    }

private:
    juce::Point<float> centre;
    float radius = 50.0f, spin = 0.0f, acc = 0.0f;
};

// =================================================================================================
//  SECRET: the creator at a tiny synthesiser (placeholder sprite - Stage 5 swaps in a portrait)
// =================================================================================================
const char* const creatorSprite[] = {
    "..ggggg..",
    ".gkkkkkg.",
    "hkkkkkkkh",
    "hkssssskh",
    "hsesssesh",
    ".sssssss.",
    "..sssss..",
    "...sss...",
    ".ccccccc.",
    "ccccccccc",
    "cc.ccc.cc",
    "ss.ccc.ss",
};

juce::Colour creatorPalette (char ch)
{
    switch (ch)
    {
        case 'g': return juce::Colour (0xffc9a45c);   // headphone band
        case 'h': return juce::Colour (0xff46eeff);   // headphone cups
        case 'k': return juce::Colour (0xff1c1410);   // hair
        case 's': return juce::Colour (0xffc89a78);   // skin
        case 'e': return juce::Colour (0xff1a1a22);   // eyes
        case 'c': return juce::Colour (0xff2a3550);   // jacket
        default:  return juce::Colours::transparentBlack;
    }
}

class CreatorCameo final : public WorldEvent
{
public:
    explicit CreatorCameo (const WorldContext& c)
    {
        layer = Layer::Back;
        duration = 16.0f;
        fadeIn = 2.5f;
        fadeOut = 2.5f;
        bx = c.screen.getX() + c.screen.getWidth() * c.rand (0.2f, 0.8f);
    }

    void update (const WorldContext& c) override
    {
        bob = smoothTo (bob, c.kick(), c.dt, 0.03f, 0.2f);

        if (c.f().kickHit)  keys[(size_t) c.rng->nextInt (4)] = 1.0f;
        if (c.f().snareHit) keys[(size_t) (9 + c.rng->nextInt (5))] = 1.0f;
        if (c.rng->nextFloat() < c.mid() * c.dt * 8.0f) keys[(size_t) (4 + c.rng->nextInt (5))] = 1.0f;

        for (auto& k : keys)
            k *= std::exp (-c.dt / 0.25f);
    }

    void draw (juce::Graphics& g, const WorldContext& c, float alpha) const override
    {
        const float px = c.px;
        const float ground = c.screen.getBottom() - 3.0f * px;
        const float synthTop = ground - 4.0f * px;

        // the synth's glow on the face
        juce::ColourGradient glowGrad (pal::cyan.withAlpha (0.18f * alpha), bx, synthTop,
                                       pal::cyan.withAlpha (0.0f), bx + 16.0f * px, synthTop, true);
        g.setGradientFill (glowGrad);
        g.fillEllipse (bx - 16.0f * px, synthTop - 16.0f * px, 32.0f * px, 32.0f * px);

        pixel::drawSprite (g, creatorSprite, 12, bx - 4.5f * px, synthTop - 11.5f * px - bob * px, px, creatorPalette, alpha);

        // synth body
        g.setColour (juce::Colour (0xff1a1d26).withAlpha (alpha));
        g.fillRect (bx - 14.0f * px, synthTop, 28.0f * px, 4.0f * px);
        g.setColour (pal::gold.withAlpha (0.7f * alpha));
        g.fillRect (bx - 14.0f * px, synthTop, 28.0f * px, juce::jmax (1.0f, px * 0.3f));

        // keys (light up with kicks / mids / snares)
        for (int i = 0; i < 14; ++i)
        {
            const float kx = bx - 14.0f * px + (float) i * 2.0f * px + 0.2f * px;
            const float lit = keys[(size_t) i];
            g.setColour (juce::Colour (0xffb9bdc9).interpolatedWith (pal::cyan, lit).withAlpha (alpha * (0.55f + 0.45f * lit)));
            g.fillRect (kx, synthTop + 1.5f * px, 1.6f * px, 2.2f * px);
        }

        // status LEDs
        for (int i = 0; i < 3; ++i)
        {
            const float on = 0.4f + 0.6f * std::abs (std::sin (age * (1.5f + (float) i)));
            g.setColour ((i == 1 ? pal::amber : pal::cyan).withAlpha (alpha * on));
            g.fillRect (bx + (8.0f + (float) i * 1.6f) * px, synthTop + 0.5f * px, 0.8f * px, 0.6f * px);
        }
    }

private:
    float bx = 0.0f, bob = 0.0f;
    std::array<float, 14> keys {};
};
} // namespace

// =================================================================================================
//  EVENT LIBRARY
//  rarity, themes, state affinity (dream / pulse / chaos), cooldown (s), allowed in Minimal
// =================================================================================================
std::vector<EventDef> createEventLibrary()
{
    return {
        { "shooting-star", Rarity::Common,   themeAny,                 1.0f, 0.7f,  0.5f,  6.0f,   true,  &makeEvent<ShootingStar> },
        { "rune",          Rarity::Common,   themeNorse | themeMystic, 0.3f, 1.0f,  1.0f,  5.0f,   true,  &makeEvent<RuneFlash> },
        { "aeroplane",     Rarity::Uncommon, themeDream | themeMystic, 1.0f, 0.6f,  0.15f, 30.0f,  false, &makeEvent<Aeroplane> },
        { "rain-cloud",    Rarity::Uncommon, themeDream | themeNorse,  1.0f, 0.5f,  0.3f,  35.0f,  false, &makeEvent<RainCloud> },
        { "ravens",        Rarity::Uncommon, themeNorse | themeDream,  0.5f, 1.0f,  0.6f,  30.0f,  false, &makeEvent<Ravens> },
        { "bench",         Rarity::Rare,     themeDream,               1.0f, 0.25f, 0.0f,  70.0f,  false, &makeEvent<BenchScene> },
        { "longship",      Rarity::Rare,     themeNorse,               0.5f, 1.0f,  0.7f,  60.0f,  false, &makeEvent<Longship> },
        { "eye-pyramid",   Rarity::Rare,     themeMystic,              0.1f, 0.6f,  1.0f,  60.0f,  false, &makeEvent<EyePyramid> },
        { "whale",         Rarity::VeryRare, themeNorse | themeDream,  1.0f, 0.5f,  0.2f,  180.0f, false, &makeEvent<Whale> },
        { "portal",        Rarity::VeryRare, themeMystic,              0.2f, 0.6f,  1.0f,  180.0f, false, &makeEvent<Portal> },
        { "creator",       Rarity::Secret,   themeAny,                 1.0f, 1.0f,  0.4f,  300.0f, false, &makeEvent<CreatorCameo> },
    };
}
}

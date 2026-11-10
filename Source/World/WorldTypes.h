#pragma once

#include <juce_graphics/juce_graphics.h>
#include "../AudioAnalyser.h"
#include "../Oscilloscope.h"
#include "../Util.h"
#include "Particles.h"
#include <array>
#include <memory>
#include <vector>

namespace token
{
enum class Layer  { Back, Mid, Front };     // Back/Mid draw beneath the oscilloscope, Front above it
enum class Rarity { Common, Uncommon, Rare, VeryRare, Secret };
enum ThemeBits    { themeDream = 1, themeMystic = 2, themeNorse = 4, themeAny = 7 };

/** Everything a scene needs to know about the world this frame. */
struct WorldContext
{
    juce::Rectangle<float> screen;
    float px = 3.0f;                 // size of one pixel-art pixel on screen
    float time = 0.0f, dt = 1.0f / 60.0f;
    float react = 0.65f, chaos = 0.45f, glow = 0.7f;
    bool strip = false;

    const AudioFeatures* audio = nullptr;
    const Oscilloscope* scope = nullptr;
    ParticleSystem* particles = nullptr;
    juce::Random* rng = nullptr;
    std::array<float, 3> theme { 1.0f, 0.0f, 0.0f };   // Dream / Mystic / Norse weights

    const AudioFeatures& f() const noexcept { return *audio; }
    float rand (float lo, float hi) const noexcept { return lo + rng->nextFloat() * (hi - lo); }

    // audio values scaled by the REACT control
    float reactScale() const noexcept { return 0.35f + 1.3f * react; }
    float bass()  const noexcept { return juce::jlimit (0.0f, 1.0f, audio->bass  * reactScale()); }
    float mid()   const noexcept { return juce::jlimit (0.0f, 1.0f, audio->mid   * reactScale()); }
    float high()  const noexcept { return juce::jlimit (0.0f, 1.0f, audio->high  * reactScale()); }
    float kick()  const noexcept { return juce::jlimit (0.0f, 1.0f, audio->kick  * reactScale()); }
    float snare() const noexcept { return juce::jlimit (0.0f, 1.0f, audio->snare * reactScale()); }
    float chaosState() const noexcept { return audio->stateWeights[2]; }
};

/** Base class for every living-world scene / event. */
class WorldEvent
{
public:
    virtual ~WorldEvent() = default;

    virtual void update (const WorldContext&) {}
    virtual void draw (juce::Graphics&, const WorldContext&, float alpha) const = 0;

    void advance (const WorldContext& c) { age += c.dt; update (c); }

    float envelope() const noexcept
    {
        const float in  = fadeIn  > 0.0f ? age / fadeIn : 1.0f;
        const float out = fadeOut > 0.0f ? (duration - age) / fadeOut : 1.0f;
        return smoothstep (juce::jmin (in, out, 1.0f));
    }

    bool isFinished() const noexcept { return age >= duration; }

    Layer layer = Layer::Mid;
    float age = 0.0f, duration = 10.0f, fadeIn = 1.5f, fadeOut = 1.5f;
    int defIndex = -1;
};

using EventFactory = std::unique_ptr<WorldEvent> (*) (const WorldContext&);

/** One entry in the event library: how rare it is, where/when it belongs, how to make one. */
struct EventDef
{
    const char* name;
    Rarity rarity;
    int themes;                       // ThemeBits mask
    float wDream, wPulse, wChaos;     // affinity to each energy state
    float cooldown;                   // seconds before it may appear again
    bool allowMinimal;                // may appear in the Minimal scene
    EventFactory make;
    float lastSpawn = -1.0e6f;
};

std::vector<EventDef> createEventLibrary();
}

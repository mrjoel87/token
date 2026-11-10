#pragma once

#include <juce_graphics/juce_graphics.h>
#include "ArtKit.h"
#include "../AudioAnalyser.h"
#include "../Theme.h"
#include <array>
#include <vector>

namespace token::art
{
struct ArtContext
{
    juce::Rectangle<float> area;      // where artwork may appear
    float time = 0.0f, dt = 1.0f / 60.0f;
    bool enabled = true;              // false in SCOPE ONLY
    bool artOnly = false;             // ART ONLY: more room, more presence
    bool strip = false;
    float intensity = 0.65f;
    float beatPhase = 0.0f;
    const AudioFeatures* audio = nullptr;
    look::Palette palette;
    const look::Look* theme = nullptr;
};

/**
    Chooses and animates the artwork.

    - Shuffle bag: every element in the library is shown once before any element can return.
      From the front of the bag, a few candidates are scored against the music (quiet / bass /
      melodic / energetic), the recent history and the free space, then one is picked with
      weighted randomness. Rare elements are skipped most of the time.
    - At most one main element plus one or two small companions are alive at once.
    - Varied placement, scale, mirroring, drift and entrance style; smooth fades in and out.
    - Drops cut the current piece short and bring in something more dramatic; breakdowns
      leave longer, sparser gaps and favour gentle scenes.
*/
class ArtEngine
{
public:
    ArtEngine();

    void update (const ArtContext&);
    void draw (juce::Graphics&, const ArtContext&) const;

    int librarySize() const noexcept;
    juce::String currentName() const;

private:
    struct Active
    {
        int spec = -1;
        float age = 0.0f, life = 10.0f, fadeIn = 2.0f, fadeOut = 2.5f, leaveAge = 0.0f;
        bool leaving = false, hero = false, flip = false;
        int entrance = 0;
        float driftX = 0.0f, driftY = 0.0f;
        juce::uint32 seed = 1;
        juce::Rectangle<float> box;

        float alpha() const noexcept;
        bool finished() const noexcept { return leaving && leaveAge >= fadeOut; }
    };

    int pick (const ArtContext&, bool hero, bool dramatic);
    void refillBag();
    void spawn (int specIndex, const ArtContext&, bool hero, bool dramatic);
    juce::Rectangle<float> place (const Spec&, const ArtContext&);
    float fit (const Spec&) const;

    std::vector<Active> active;
    std::vector<int> bag, history;
    juce::Random rng;

    std::array<float, 4> profile {};   // quiet, bass, melodic, energetic
    float heroTimer = 1.5f, companionTimer = 8.0f, lastDrop = 0.0f;
};
}

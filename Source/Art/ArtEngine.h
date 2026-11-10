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
    juce::Rectangle<float> area;      // the artwork canvas (safe bounds inside the screen)
    float time = 0.0f, dt = 1.0f / 60.0f;
    bool enabled = true;              // false in SCOPE ONLY
    bool artOnly = false;             // ART ONLY: more room, more presence
    bool strip = false;
    bool locked = false;              // LOCK: hold the current scene
    float intensity = 0.65f;
    float speed = 1.0f;               // SPEED multiplier (see token::speedFactor)

    double beats = 0.0;               // musical clock
    double beatsPerBar = 4.0;
    float beatPhase = 0.0f, barPhase = 0.0f;
    bool hostPlaying = false;

    const AudioFeatures* audio = nullptr;
    look::Palette palette;
    const look::Look* theme = nullptr;
};

/**
    Chooses and animates the artwork.

    - Shuffle bag: every element is shown once before any can return; candidates from the front of
      the bag are scored against the music (quiet / bass / melodic / energetic) and picked with
      weighted randomness. Rare elements are skipped most of the time.
    - Scene changes land on bar lines (or beats) so they feel musical; SPEED shortens or lengthens
      lifetimes, gaps and motion; LOCK keeps the current scene alive (still animating and reacting).
    - Geometry turns in eased steps locked to the beat, everything pulses with kicks and swells
      with the bass; breakdowns soften and slow the display.
    - Elements are placed inside a safe canvas with balanced, non-overlapping composition.
*/
class ArtEngine
{
public:
    ArtEngine();

    void update (const ArtContext&);
    void draw (juce::Graphics&, const ArtContext&, float alphaScale) const;

    int librarySize() const noexcept;
    juce::String currentName() const;

private:
    struct Active
    {
        int spec = -1;
        float age = 0.0f;              // real seconds (fades)
        float clock = 0.0f;            // animation time, scaled by SPEED
        float life = 10.0f;            // in animation time
        float fadeIn = 2.0f, fadeOut = 2.5f, leaveAge = 0.0f, waitTime = 0.0f, travel = 0.0f;
        bool leaving = false, wantsLeave = false, hero = false, flip = false;
        int entrance = 0;
        float spinDir = 1.0f, driftX = 0.0f, driftY = 0.0f;
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
    void startLeaving (Active&, float fadeSeconds);

    std::vector<Active> active;
    std::vector<int> bag, history;
    juce::Random rng;

    std::array<float, 4> profile {};   // quiet, bass, melodic, energetic
    float heroTimer = 0.6f, companionTimer = 6.0f, lastDrop = 0.0f, breakdown = 0.0f;
    double lastBeats = 0.0;
    long long lastBeatIndex = 0, lastBarIndex = 0;
    bool firstUpdate = true;
};
}

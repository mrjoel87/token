#pragma once

#include <juce_graphics/juce_graphics.h>
#include "SceneKit.h"
#include "../AudioAnalyser.h"
#include "../Oscilloscope.h"
#include "../Parameters.h"
#include "../World/Particles.h"
#include <array>
#include <vector>

namespace token::tx
{
struct TxSettings
{
    std::array<bool, numCategories> categories { true, true, true, true, true };
    SyncMode sync = SyncMode::Auto;
    SpeedMode speed = SpeedMode::Auto;
    float density = 0.5f, mystery = 0.5f, visibility = 0.6f;
    bool rare = true;
};

struct Transport
{
    double bpm = 120.0, ppq = 0.0, beatsPerBar = 4.0;
    bool playing = false;
};

struct TxContext
{
    juce::Rectangle<float> screen;
    float px = 3.0f, time = 0.0f, dt = 1.0f / 60.0f;
    const AudioFeatures* audio = nullptr;
    const Oscilloscope* scope = nullptr;
    ParticleSystem* particles = nullptr;
    TxSettings settings;
    Transport transport;
};

/**
    The Hidden Transmission engine: chooses scenes from the library according to the music,
    changes them on bar boundaries (or freely), renders them off-screen and composites them
    INTO the signal - a faint ghost over the whole screen, strongly revealed inside the waveform's
    envelope, torn by analogue interference, materialising and dissolving into particles.
*/
class TransmissionEngine
{
public:
    TransmissionEngine();

    void update (const TxContext&);
    void draw (juce::Graphics&, const TxContext&) const;

    /** e.g. "RX 042  /  LOST" - empty when nothing is showing. */
    juce::String getStatus() const { return status; }

    /** Inks for the scenes, taken from the current theme palette. */
    static Ink inkFromPalette();

private:
    struct Slot
    {
        int scene = -1;
        juce::uint32 seed = 1;
        float age = 0.0f, fadeIn = 2.0f, fadeOut = 2.0f, life = 1.0e9f, leaveAge = 0.0f;
        bool leaving = false, apparition = false, sampledIn = false, sampledOut = false;
        juce::Image image;

        float alpha() const noexcept;
        bool finished() const noexcept { return leaving && leaveAge >= fadeOut; }
    };

    void startScene (int index, const TxContext&, float fade, bool apparition, float life);
    int chooseScene (const TxContext&, bool apparition);
    float intervalSeconds (const TxContext&) const;
    float autoBars (const TxContext&) const;
    void renderSlot (Slot&, const TxContext&);
    void releaseParticles (const Slot&, const TxContext&, bool dissolve);
    juce::Rectangle<float> destRect (const TxContext&, const juce::Image&) const;
    void drawStrips (juce::Graphics&, const Slot&, juce::Rectangle<float> dest, float alpha, float amplitude,
                     float time, bool materialise) const;

    std::vector<Slot> slots;
    std::vector<int> history;
    std::vector<float> weights;
    juce::Random rng;

    std::array<float, 5> profile {};   // ambient, rhythm, sparse, dark, intense
    float sinceChange = 0.0f, apparitionTimer = 8.0f, freeBeat = 0.0f, beatNow = 0.0f;
    double lastBarIndex = -1.0, lastUnit = -1.0;
    float currentBars = 4.0f;
    bool started = false;
    juce::String status;
};
}

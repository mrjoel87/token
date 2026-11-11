#pragma once

#include "SignalFrame.h"

namespace token::engine
{
/**
    ALIGNMENT: rare moments where everything settles into one extraordinary formation and then
    dissolves back into the world.

    Triggers (each only a chance, never guaranteed):
      - the release of a drop
      - a strong kick near the top of a build-up
      - a calm stretch after activity
      - a 16-bar phrase boundary with the host playing and the track busy
    A cooldown (45 s) and a warm-up (20 s after loading) keep it special. Can be switched off.

    Forms (round-robin): mandala, rune structure, geometric eye, interlocking circles. Each is built
    from a snapshot of the waveform at the trigger, then breathes with the live audio.
*/
class AlignmentDirector
{
public:
    void update (const SignalFrame&, bool enabled);
    void draw (juce::Graphics&, const SignalFrame&) const;

    /** 0..1 how strongly an alignment is on screen (the worlds dim beneath it). */
    float amount() const noexcept { return level; }

private:
    void trigger (const SignalFrame&);
    void drawMandala (juce::Graphics&, const SignalFrame&, juce::Point<float>, float R, float a) const;
    void drawRune (juce::Graphics&, const SignalFrame&, juce::Point<float>, float R, float a) const;
    void drawEye (juce::Graphics&, const SignalFrame&, juce::Point<float>, float R, float a) const;
    void drawInterlock (juce::Graphics&, const SignalFrame&, juce::Point<float>, float R, float a) const;

    static constexpr float rise = 1.4f, hold = 2.8f, fall = 2.4f;

    std::array<float, SignalFrame::N> shape {};
    float age = -1.0f;            // < 0: idle
    float level = 0.0f;
    float cooldown = 20.0f;       // warm-up before the first one
    float calmTimer = 0.0f, busyTimer = 0.0f;
    float lastDrop = 0.0f;
    long long lastPhrase = -1;
    int form = 0;
    juce::Random rng { 0x70737e };

    mutable juce::Path path;
};
}

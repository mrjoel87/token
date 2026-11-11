#pragma once

#include "SignalFrame.h"

namespace token::engine
{
/**
    SIGNAL: the clean, audio-focused world.
      - the live trace (zero-crossing triggered), weighted by the bass, pulsing on kicks
      - its recent history as ribbons of light: in BOX they recede into a waveform tunnel towards
        a vanishing point; in STRIP they trail upwards like a flowing ribbon
      - snares split the trace briefly into two fine coloured echoes
      - a restrained mirrored spectrum floor, its fine points lit by the highs
*/
class SignalWorld
{
public:
    void update (const SignalFrame&);
    void draw (juce::Graphics&, const SignalFrame&, float alpha) const;

private:
    static constexpr int layers = 18;
    static constexpr int points = 160;

    std::array<std::array<float, points>, layers> history {};
    int head = 0, filled = 0;
    float pushClock = 0.0f, split = 0.0f, zoom = 0.0f;

    mutable juce::Path trace, ribbon, spectrum;
};
}

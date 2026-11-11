#pragma once

#include "SignalFrame.h"
#include "../Art/ArtKit.h"
#include <vector>

namespace token::engine
{
/**
    DREAM: a quiet, nostalgic night world built around the signal.
      - the low-passed waveform IS the horizon (hills breathe with the bass), mirrored as a faint
        reflection below; fine detail glints along its edge with the highs
      - a small moon that crosses the sky over minutes, a handful of stars twinkling with hi-hats,
        soft clouds that thicken with sustained sound
      - rare pixel-art moments (a couple on a bench, a distant aeroplane, a rain cloud, a lit
        window, a lantern boat ...): one at a time, small, placed on the horizon or in the sky,
        drawn from a shuffled list so nothing repeats until everything has appeared
*/
class DreamWorld
{
public:
    DreamWorld();
    void update (const SignalFrame&);
    void draw (juce::Graphics&, const SignalFrame&, float alpha) const;

private:
    struct Moment
    {
        int spec = -1;
        float age = 0.0f, life = 16.0f;
        float u = 0.5f;           // horizontal position (0..1) for horizon pieces
        bool flip = false;
        juce::uint32 seed = 1;
        float alpha() const;
    };

    float horizonY (const SignalFrame&, float u) const;
    void startMoment (const SignalFrame&);

    std::vector<int> pool, bag;
    Moment moment;
    bool hasMoment = false;
    float gap = 6.0f;
    juce::Random rng;

    mutable juce::Path hills, reflection;
};
}

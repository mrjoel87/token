#pragma once

#include "Theme.h"
#include "Transport.h"
#include "AudioAnalyser.h"

namespace token
{
/**
    MANUAL: a fixed, carefully balanced palette.
    BEAT COLOUR: palettes evolve in time with the host tempo - a gentle pulse on every beat and a
    smooth crossfade to the next palette over musical bars (every 1, 2 or 4 bars depending on the
    music's energy). When Live is stopped it keeps time from the host BPM.
*/
class ColourEngine
{
public:
    void update (float dt, const AudioFeatures&, const Transport&, bool beatMode, bool lightTheme);

    const look::Palette& palette() const noexcept { return current; }
    float pulse() const noexcept { return beatPulse; }        // 0..1, gentle
    float beatPhase() const noexcept { return phase; }        // 0..1 within the beat

private:
    look::Palette current { juce::Colour (0xff8eeeff), juce::Colour (0xff7b5cff), juce::Colour (0xffffb35c) };
    double freeBeat = 0.0, segmentStart = 0.0, segmentLength = 8.0;
    int paletteIndex = 0;
    float beatPulse = 0.0f, phase = 0.0f;
    bool initialised = false;
};
}

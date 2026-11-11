#pragma once

#include "Theme.h"
#include "Transport.h"
#include "AudioAnalyser.h"

namespace token
{
/**
    The musical clock and the artwork's colours.

    Clock: while Live's transport runs, the beat position comes from the host (beatValid = true).
    When it is stopped there is no reliable beat grid, so none is invented: beatValid is false and
    the visuals use onsets and envelopes instead. A free-running "evolution" clock always runs.

    COLOUR: four fixed studio palettes, or PULSE, which moves through them in time with the music
    (every 2 or 4 bars with the host, otherwise on strong kicks after a while) and brightens on
    each beat / kick.
*/
class ColourEngine
{
public:
    void update (float dt, const AudioFeatures&, const Transport&, int paletteIndex);

    const look::Palette& palette() const noexcept { return current; }
    float pulse() const noexcept { return beatPulse; }
    bool beatValid() const noexcept { return hostSynced; }
    double beats() const noexcept { return beat; }
    double beatsPerBar() const noexcept { return barLength; }
    float beatPhase() const noexcept { return phase; }
    float barPhase() const noexcept { return barPhaseValue; }

private:
    look::Palette current { juce::Colour (0xff9ff2ff), juce::Colour (0xff2f5bff), juce::Colour (0xffffb35c) };
    double beat = 0.0, barLength = 4.0, segmentStart = 0.0;
    int pulseIndex = 0;
    float beatPulse = 0.0f, phase = 0.0f, barPhaseValue = 0.0f, sinceSwitch = 0.0f;
    bool hostSynced = false, initialised = false;
};
}

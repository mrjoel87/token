#pragma once

#include "Theme.h"
#include "Transport.h"
#include "AudioAnalyser.h"

namespace token
{
/**
    The musical clock and the artwork's colours.

    Beat clock: follows Ableton's song position while the transport runs. When it is stopped the
    clock keeps running at the host tempo and re-aligns itself to detected kick onsets, so beat-locked
    motion still lines up with the music.

    MANUAL: a fixed, carefully balanced palette (with a gentle kick bloom handled by the visuals).
    BEAT COLOUR: a visible pulse on every beat (accent bleeds into the line, stronger on the downbeat),
    a slow hue drift through each bar and a crossfade to the next palette every 1, 2 or 4 bars
    depending on the music's energy. Applied to the scope, the spectrum and the artwork.
*/
class ColourEngine
{
public:
    void update (float dt, const AudioFeatures&, const Transport&, bool beatMode, bool lightTheme);

    const look::Palette& palette() const noexcept { return current; }
    float pulse() const noexcept { return beatPulse; }            // 0..1 (beat colour pulse)
    float beatPhase() const noexcept { return phase; }            // 0..1 within the beat
    double beats() const noexcept { return beat; }                // continuous beat count
    double beatsPerBar() const noexcept { return barLength; }
    float barPhase() const noexcept { return barPhaseValue; }     // 0..1 within the bar
    bool synced() const noexcept { return hostSynced; }

private:
    look::Palette current { juce::Colour (0xff8eeeff), juce::Colour (0xff7b5cff), juce::Colour (0xffffb35c) };
    double beat = 0.0, barLength = 4.0, segmentStart = 0.0, segmentLength = 8.0;
    int paletteIndex = 0;
    float beatPulse = 0.0f, phase = 0.0f, barPhaseValue = 0.0f;
    bool initialised = false, hostSynced = false;
};
}

#pragma once

#include "SignalFrame.h"

namespace token::engine
{
/**
    SACRED: the signal folded into k-fold symmetry.
      - bass ring: the low-passed waveform wrapped around a circle and mirrored into every fold
      - petals: their outlines ARE the (smoothed) waveform, mirrored left/right; sustained sound
        unfolds them, hi-hats add a fine vein
      - frame: polygon + star that turns in eased steps on host beats (or on kicks without a host)
      - rune spokes: branch patterns read from the waveform itself, flashing on snares
      - core: pulses with kicks; drops bloom outwards; quiet passages breathe and dim
    In STRIP the composition becomes a frieze of counter-rotating rosettes, sized to the height.
*/
class SacredWorld
{
public:
    void update (const SignalFrame&);
    void draw (juce::Graphics&, const SignalFrame&, float alpha) const;

private:
    void drawRosette (juce::Graphics&, const SignalFrame&, juce::Point<float> c, float R, float dir,
                      bool primary, float alpha) const;

    int folds = 8, previousFolds = 8;
    float foldBlend = 1.0f;
    float rotation = 0.0f, stepTarget = 0.0f, step = 0.0f;
    float spokeFlash = 0.0f, bloom = 0.0f, unfold = 0.3f, breathe = 0.0f;
    float phraseClock = 0.0f, glyphClock = 0.0f;
    std::array<int, 6> glyph { 1, 0, -1, 1, 0, 1 };
    long long lastPhrase = -1, lastGlyphBar = -1;
    int foldCursor = 0;

    mutable juce::Path ring, petals, frame, spokes;
};
}

#pragma once

#include "Sketch.h"
#include "SignalFrame.h"

namespace token::engine
{
/** How the waveform rides along a sketch's lines. */
struct PenStyle
{
    float amp = 6.0f;          // waveform displacement in pixels
    float cycles = 4.0f;       // how many times the waveform repeats along the whole drawing
    float phase = 0.0f;        // slides the waveform along the lines
    float reveal = 1.0f;       // 0..1: draws each stroke only up to this fraction (the "drawing on" effect)
    float stepPx = 2.5f;       // sample spacing along the lines
    int maxPoints = 1800;      // budget for the whole drawing
    bool useFine = false;      // ride the high-frequency detail instead of the full waveform
};

/**
    The heart of TOTEM 2: turns any sketch into an oscilloscope trace. Each line is resampled along
    its length and pushed sideways by the live waveform, so every symbol, shape and letter is the
    signal itself. The result is a reusable Path, drawn with a restrained glow.
*/
class ScopePen
{
public:
    ScopePen();

    /** Builds `out` from the sketch (unit space) mapped to the screen by `xf`. */
    void build (const Sketch&, const juce::AffineTransform& xf, const PenStyle&, const SignalFrame&, juce::Path& out);

    /** Core line + up to two halo passes (by quality), optionally under a transform. */
    static void glow (juce::Graphics&, const juce::Path&, juce::Colour core, juce::Colour halo, float width,
                      float amount, int quality, const juce::AffineTransform& xf = {});

private:
    std::vector<juce::Point<float>> scr;
    std::vector<float> strokeLen;
};
}

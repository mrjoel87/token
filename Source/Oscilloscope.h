#pragma once

#include <juce_graphics/juce_graphics.h>
#include "AudioAnalyser.h"
#include "ScopeBuffers.h"
#include <array>
#include <vector>

namespace token
{
struct ScopeSettings
{
    bool scrolling = false, stereo = false, autoGain = true, spectrum = true, strip = false;
    float glow = 0.7f, react = 0.65f;
};

/**
    The real, sample-driven oscilloscope.
    - Triggered mode: rising zero-crossing trigger (low-passed, with hysteresis and sub-sample
      interpolation) for a stable standing waveform.
    - Scrolling mode: min/max envelope of the last ~1-1.6 s, bucket-aligned so it scrolls without shimmer.
    Also exposes a smoothed version of the wave so the living world can ride on it.
*/
class Oscilloscope
{
public:
    void update (const ScopeHistory&, const AudioFeatures&, const ScopeSettings&,
                 juce::Rectangle<float> area, double sampleRate, float dt);

    void draw (juce::Graphics&, const AudioFeatures&, const ScopeSettings&) const;
    void drawSpectrum (juce::Graphics&, const AudioFeatures&, juce::Rectangle<float> area, float glow) const;

    /** Smoothed waveform height (screen y) at screen x - for objects that sail / fly on the wave. */
    float waveYAt (float x) const noexcept;
    /** Smoothed upper envelope (screen y) near screen x - the "peaks" of the landscape. */
    float envelopeTopAt (float x) const noexcept;

    /** Closed region hugging the signal's envelope (plus a margin) - the window images are revealed through. */
    juce::Path signalBand (float margin) const;

    float centreY() const noexcept { return laneGeometry (0).centre; }
    juce::Rectangle<float> getArea() const noexcept { return area; }
    juce::Colour lineColour (const AudioFeatures&) const;

private:
    struct Lane { float centre, half; };
    static constexpr int maxTrails = 12;

    Lane laneGeometry (int lane) const noexcept;
    float sampleColumn (const std::vector<float>& v, float x) const noexcept;
    void resizeBuffers (int newCols);
    void buildLinePath (juce::Path&, const std::vector<float>&, float centre, float halfHeight) const;

    juce::Rectangle<float> area;
    int cols = 0;
    bool scrolling = false, stereo = false, lastScrolling = false, lastStereo = false;

    std::vector<float> rawA, rawB, minV, maxV, smoothWave, smoothEnv;
    std::vector<float> lpScratch;
    std::vector<int> negScratch;

    juce::Path lineA, lineB, envelopeFill;
    std::array<juce::Path, maxTrails> trails;
    int trailHead = 0, trailCount = 0;
};
}

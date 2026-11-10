#pragma once

#include <juce_graphics/juce_graphics.h>
#include "AudioAnalyser.h"
#include "Parameters.h"
#include "ScopeBuffers.h"
#include "Theme.h"
#include <array>
#include <vector>

namespace token
{
struct VisualSettings
{
    VisualMode mode = VisualMode::Classic;
    float intensity = 0.65f;
    bool strip = false;
};

/**
    The artwork - drawn entirely from the incoming audio:

    CLASSIC  horizontal oscilloscope (zero-crossing triggered), weighted by the bass, pulsing on kicks,
             splitting into several luminous lines in drops and softening in breakdowns.
    ORBIT    the waveform wrapped around a circle that breathes with the bass, deforms, rotates with
             the mids and echoes outwards; in energetic passages it gains rotational symmetry.
    MIRROR   a phase-space portrait of the signal (sample against delayed sample), mirrored into a
             kaleidoscope whose symmetry grows with the music's energy.
    NONE     no oscilloscope - the spectrum takes over the screen.

    Everything here runs on the message thread; nothing touches the audio thread.
*/
class Visualiser
{
public:
    void update (const ScopeHistory&, const AudioFeatures&, const VisualSettings&,
                 juce::Rectangle<float> area, double sampleRate, float dt, float time);

    void draw (juce::Graphics&, const look::Look&, const look::Palette&, float beatPulse) const;

    void drawSpectrum (juce::Graphics&, const AudioFeatures&, juce::Rectangle<float> area,
                       const look::Look&, const look::Palette&, bool prominent) const;

private:
    void extractTriggered (const ScopeHistory&, float gain, float peak, double sampleRate,
                           int displayLen, std::vector<float>& out, int n);
    void pushTrail (const juce::Path&);

    void drawClassic (juce::Graphics&, const look::Look&, const look::Palette&, float beatPulse) const;
    void drawOrbit   (juce::Graphics&, const look::Look&, const look::Palette&, float beatPulse) const;
    void drawMirror  (juce::Graphics&, const look::Look&, const look::Palette&, float beatPulse) const;
    void strokeLuminous (juce::Graphics&, const juce::Path&, juce::Colour core, juce::Colour halo,
                         float width, float glowAmount, bool lightTheme) const;

    juce::Rectangle<float> area;
    VisualMode mode = VisualMode::Classic, builtMode = VisualMode::None;
    bool strip = false;
    float time = 0.0f, k = 1.0f;

    // reactions (all smoothed)
    float pulse = 0.0f, weight = 0.0f, split = 0.0f, detail = 0.0f, glow = 0.5f;
    float flow = 0.0f, calm = 0.0f, symmetry = 0.0f, rotation = 0.0f, radius = 0.0f;

    std::vector<float> raw, disp, high, lpScratch;
    std::vector<int> negScratch;
    std::vector<juce::Point<float>> phasePts;

    juce::Path current, detailPath;
    static constexpr int maxTrails = 8;
    std::array<juce::Path, maxTrails> trails;
    int trailHead = 0, trailCount = 0;

    juce::Point<float> centre;
    float halfHeight = 1.0f, orbitR = 1.0f, orbitAmp = 1.0f;

    std::array<float, AudioFeatures::numSpectrumBands> peaks {};
};
}

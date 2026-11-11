#pragma once

#include <juce_dsp/juce_dsp.h>
#include "ScopeBuffers.h"
#include <array>

namespace token
{
enum class EnergyState { Dream = 0, Pulse = 1, Chaos = 2 };

/** Everything the visuals know about the music, updated once per frame on the UI thread. */
struct AudioFeatures
{
    static constexpr int numSpectrumBands = 48;

    float rms = 0.0f, rmsDb = -100.0f, peak = 0.0f;
    float bass = 0.0f, mid = 0.0f, high = 0.0f;          // 0..1 smoothed band levels
    float kick = 0.0f, snare = 0.0f;                     // 0..1 decaying transient envelopes
    bool kickHit = false, snareHit = false;              // true only on the frame of an onset
    float transient = 0.0f;
    float onsetRate = 0.0f;                              // onsets per second (smoothed)
    float sustain = 0.0f;                                // loud but not percussive
    float intensity = 0.0f, energyDelta = 0.0f;
    float silence = 1.0f;                                // 1 when nothing is playing
    float scopeGain = 1.0f;                              // automatic visual gain
    float loudness = 0.0f;                               // 0..1 from RMS dB
    float centroid = 0.5f;                               // 0..1 spectral brightness (log 100 Hz .. 8 kHz)
    float spectralMotion = 0.0f;                         // how fast the brightness is changing

    // Adaptive (relative) band levels: each band compared with its own recent maximum, so quiet
    // mixes still move the visuals and loud mixes don't saturate them.
    float bassRel = 0.0f, midRel = 0.0f, highRel = 0.0f;
    float hat = 0.0f;                                    // hi-hat / fine percussion envelope
    bool hatHit = false;
    float drop = 0.0f;                                   // 1 right after a sudden rise in energy, decays over seconds
    float breakdown = 0.0f;                              // > 0 while energy is falling away from a louder section
    float build = 0.0f;                                  // > 0 while energy has been climbing for several seconds

    EnergyState state = EnergyState::Dream;
    EnergyState previousState = EnergyState::Dream;
    bool stateChanged = false;
    std::array<float, 3> stateWeights { 1.0f, 0.0f, 0.0f };   // smoothed Dream / Pulse / Chaos

    std::array<float, numSpectrumBands> spectrum {};
};

/**
    Lightweight music analysis: RMS/peak, FFT bands, spectral-flux onsets (kick-like / snare-like),
    intensity and the Dream / Pulse / Chaos state machine with hysteresis.
    Runs on the UI thread from the history buffer - never on the audio thread.
*/
class AudioAnalyser
{
public:
    AudioAnalyser();

    void process (const ScopeHistory& history, int numNewSamples, double sampleRate, float dt);
    const AudioFeatures& features() const noexcept { return f; }

private:
    struct OnsetDetector
    {
        float mean = 0.0f, var = 0.0f, sinceLast = 10.0f;
        float process (float flux, float dt, float minGap, float sensitivity, float floor) noexcept;
    };

    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder;

    juce::dsp::FFT fft { fftOrder };
    std::array<float, fftSize * 2> fftData {};
    std::array<float, fftSize> window {};
    std::array<float, fftSize / 2> prevMag {};

    OnsetDetector kickDetector, snareDetector, hatDetector;
    AudioFeatures f;

    float onsetEnergy = 0.0f, longTermDb = -40.0f, stateHeld = 0.0f;
    float intensityPrev = 0.0f, peakEnv = 0.05f, noDataTime = 0.0f;
    float bassEnv = 0.3f, midEnv = 0.3f, highEnv = 0.3f;
    float slowIntensity = 0.0f, dropCooldown = 0.0f, glacialIntensity = 0.0f;
};
}

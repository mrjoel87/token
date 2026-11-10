#include "AudioAnalyser.h"
#include "Util.h"
#include <algorithm>
#include <cmath>

namespace token
{
namespace
{
    float level01 (float amplitude, float floorDb, float ceilDb) noexcept
    {
        const float db = juce::Decibels::gainToDecibels (amplitude, -120.0f);
        return juce::jlimit (0.0f, 1.0f, (db - floorDb) / (ceilDb - floorDb));
    }
}

AudioAnalyser::AudioAnalyser()
{
    for (int i = 0; i < fftSize; ++i)
        window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (fftSize - 1));
}

float AudioAnalyser::OnsetDetector::process (float flux, float dt, float minGap, float sensitivity, float floor) noexcept
{
    sinceLast += dt;

    const float sd = std::sqrt (juce::jmax (0.0f, var));
    const float margin = sensitivity * sd + floor;
    float strength = 0.0f;

    if (flux > mean + margin && sinceLast >= minGap)
    {
        const float ratio = (flux - mean) / juce::jmax (1.0e-6f, margin);
        strength = juce::jlimit (0.25f, 1.0f, 0.25f + 0.25f * (ratio - 1.0f));
        sinceLast = 0.0f;
    }

    // adaptive threshold statistics (exponential mean / variance, ~1.2 s memory)
    const float a = 1.0f - std::exp (-dt / 1.2f);
    const float diff = flux - mean;
    mean += a * diff;
    var  += a * (diff * diff - var);

    return strength;
}

void AudioAnalyser::process (const ScopeHistory& h, int numNew, double sampleRate, float dt)
{
    dt = juce::jlimit (0.001f, 0.1f, dt);
    const double sr = juce::jmax (8000.0, sampleRate);
    const auto total = h.getTotal();

    noDataTime = numNew > 0 ? 0.0f : noDataTime + dt;
    const bool hasData = total > 0 && noDataTime < 0.25f;

    // ---------------------------------------------------------------- RMS / peak
    float rmsNow = 0.0f, peakNow = 0.0f;

    if (hasData)
    {
        const int n = (int) juce::jmin ((juce::int64) 1024, total);
        double sum = 0.0;

        for (auto i = total - n; i < total; ++i)
        {
            const float l = h.left (i), r = h.right (i);
            sum += 0.5 * ((double) l * l + (double) r * r);
            peakNow = juce::jmax (peakNow, std::abs (l), std::abs (r));
        }

        rmsNow = (float) std::sqrt (sum / (double) n);
    }

    f.rms   = smoothTo (f.rms, rmsNow, dt, 0.03f, 0.25f);
    f.rmsDb = juce::Decibels::gainToDecibels (f.rms, -100.0f);
    f.peak  = smoothTo (f.peak, peakNow, dt, 0.005f, 0.3f);

    // ---------------------------------------------------------------- FFT bands + flux
    float bassE = 0.0f, midE = 0.0f, highE = 0.0f, kickFlux = 0.0f, snareFlux = 0.0f;

    if (hasData)
    {
        for (int i = 0; i < fftSize; ++i)
        {
            const auto idx = total - fftSize + i;
            fftData[(size_t) i] = idx >= 0 ? h.mono (idx) * window[(size_t) i] : 0.0f;
        }
        std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);

        fft.performFrequencyOnlyForwardTransform (fftData.data());

        const float norm  = 4.0f / (float) fftSize;     // full-scale Hann-windowed sine -> ~1.0
        const float binHz = (float) (sr / fftSize);
        const int lastBin = fftSize / 2 - 1;

        auto band = [&] (float lo, float hi, float& energyOut, float* fluxOut)
        {
            const int k0 = juce::jlimit (1, lastBin, (int) std::floor (lo / binHz));
            const int k1 = juce::jlimit (k0, lastBin, (int) std::ceil (hi / binHz));
            float e = 0.0f, fl = 0.0f;

            for (int k = k0; k <= k1; ++k)
            {
                const float m = fftData[(size_t) k] * norm;
                e += m * m;
                fl += juce::jmax (0.0f, m - prevMag[(size_t) k]);
            }

            energyOut = std::sqrt (e);
            if (fluxOut != nullptr)
                *fluxOut = fl;
        };

        float unused = 0.0f;
        band (25.0f, 150.0f, bassE, nullptr);
        band (150.0f, 2500.0f, midE, nullptr);
        band (2500.0f, 14000.0f, highE, nullptr);
        band (30.0f, 140.0f, unused, &kickFlux);
        band (1500.0f, 9000.0f, unused, &snareFlux);

        // log-spaced display spectrum
        const float fMin = 30.0f;
        const float fMax = (float) juce::jmin (16000.0, sr * 0.48);
        const int nb = AudioFeatures::numSpectrumBands;

        for (int b = 0; b < nb; ++b)
        {
            const float lo = fMin * std::pow (fMax / fMin, (float) b / (float) nb);
            const float hi = fMin * std::pow (fMax / fMin, (float) (b + 1) / (float) nb);
            const int k0 = juce::jlimit (1, lastBin, (int) std::floor (lo / binHz));
            const int k1 = juce::jlimit (k0, lastBin, (int) std::ceil (hi / binHz));
            float mx = 0.0f;

            for (int k = k0; k <= k1; ++k)
                mx = juce::jmax (mx, fftData[(size_t) k] * norm);

            f.spectrum[(size_t) b] = smoothTo (f.spectrum[(size_t) b], level01 (mx, -72.0f, -6.0f), dt, 0.01f, 0.22f);
        }

        for (int k = 0; k <= lastBin; ++k)
            prevMag[(size_t) k] = fftData[(size_t) k] * norm;
    }
    else
    {
        for (auto& s : f.spectrum)
            s = smoothTo (s, 0.0f, dt, 0.3f);
    }

    f.bass = smoothTo (f.bass, hasData ? level01 (bassE, -50.0f, -8.0f)  : 0.0f, dt, 0.02f, 0.18f);
    f.mid  = smoothTo (f.mid,  hasData ? level01 (midE,  -54.0f, -10.0f) : 0.0f, dt, 0.03f, 0.22f);
    f.high = smoothTo (f.high, hasData ? level01 (highE, -60.0f, -14.0f) : 0.0f, dt, 0.02f, 0.20f);

    // ---------------------------------------------------------------- onsets
    f.kick  *= std::exp (-dt / 0.14f);
    f.snare *= std::exp (-dt / 0.10f);
    onsetEnergy *= std::exp (-dt / 2.0f);
    f.kickHit = f.snareHit = false;

    if (hasData && numNew > 0)
    {
        const float k = kickDetector.process (kickFlux, dt, 0.11f, 1.6f, 0.04f);
        const float s = snareDetector.process (snareFlux, dt, 0.08f, 1.8f, 0.05f);

        if (k > 0.0f) { f.kickHit = true;  f.kick  = juce::jmax (f.kick, k);  onsetEnergy += 1.0f; }
        if (s > 0.0f) { f.snareHit = true; f.snare = juce::jmax (f.snare, s); onsetEnergy += 1.0f; }
    }

    f.onsetRate = onsetEnergy / 2.0f;
    f.transient = smoothTo (f.transient, juce::jmax (f.kick, f.snare), dt, 0.005f, 0.1f);

    // ---------------------------------------------------------------- intensity / sustain
    const float loud = juce::jlimit (0.0f, 1.0f, (f.rmsDb + 50.0f) / 42.0f);

    if (f.rmsDb > -80.0f)
        longTermDb = smoothTo (longTermDb, f.rmsDb, dt, 12.0f);

    const float relative = juce::jlimit (-0.15f, 0.15f, (f.rmsDb - longTermDb) / 40.0f);
    const float rateNorm = juce::jlimit (0.0f, 1.0f, f.onsetRate / 6.0f);
    const float target = juce::jlimit (0.0f, 1.0f, 0.6f * loud + 0.22f * rateNorm + 0.18f * f.bass + relative);

    f.intensity   = smoothTo (f.intensity, target, dt, 0.35f, 1.4f);
    f.energyDelta = smoothTo (f.energyDelta, (f.intensity - intensityPrev) / dt, dt, 0.2f);
    intensityPrev = f.intensity;

    f.sustain = smoothTo (f.sustain, loud * (1.0f - juce::jlimit (0.0f, 1.0f, f.onsetRate / 5.0f)), dt, 1.2f, 2.0f);
    f.silence = smoothTo (f.silence, loud < 0.02f ? 1.0f : 0.0f, dt, 1.5f, 0.1f);

    // ---------------------------------------------------------------- Dream / Pulse / Chaos (hysteresis + hold)
    f.stateChanged = false;
    stateHeld += dt;

    auto go = [this] (EnergyState s)
    {
        f.previousState = f.state;
        f.state = s;
        f.stateChanged = true;
        stateHeld = 0.0f;
    };

    switch (f.state)
    {
        case EnergyState::Dream:
            if (f.intensity > 0.46f && stateHeld > 1.2f) go (EnergyState::Pulse);
            break;
        case EnergyState::Pulse:
            if (f.intensity > 0.73f && stateHeld > 1.0f)      go (EnergyState::Chaos);
            else if (f.intensity < 0.32f && stateHeld > 2.0f) go (EnergyState::Dream);
            break;
        case EnergyState::Chaos:
            if (f.intensity < 0.60f && stateHeld > 1.6f) go (EnergyState::Pulse);
            break;
    }

    for (int i = 0; i < 3; ++i)
        f.stateWeights[(size_t) i] = smoothTo (f.stateWeights[(size_t) i], i == (int) f.state ? 1.0f : 0.0f, dt, 0.9f);

    // ---------------------------------------------------------------- automatic visual gain
    peakEnv = juce::jmax (peakNow, peakEnv * std::exp (-dt / 3.0f));
    const float gainTarget = juce::jlimit (1.0f, 16.0f, 0.8f / juce::jmax (0.02f, peakEnv));
    f.scopeGain = smoothTo (f.scopeGain, gainTarget, dt, 1.2f, 0.06f);   // rise slowly, fall fast
}
}

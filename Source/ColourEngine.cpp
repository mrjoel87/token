#include "ColourEngine.h"
#include "Util.h"
#include <cmath>

namespace token
{
void ColourEngine::update (float dt, const AudioFeatures& f, const Transport& t, bool beatMode, bool lightTheme)
{
    // ------------------------------------------------------------ beat clock
    const double bpm = juce::jlimit (20.0, 400.0, t.bpm);
    barLength = juce::jmax (1.0, t.beatsPerBar);
    hostSynced = t.playing;

    if (t.playing)
    {
        beat = t.ppq;
    }
    else
    {
        beat += (double) dt * bpm / 60.0;

        // no song position: keep the clock in step with the kicks we can hear
        if (f.kickHit)
        {
            const double fr = beat - std::floor (beat);
            if (fr > 0.7)       beat = std::ceil (beat);
            else if (fr < 0.3)  beat = std::floor (beat);
        }
    }

    phase = (float) (beat - std::floor (beat));
    const double inBar = std::fmod (beat, barLength);
    barPhaseValue = (float) ((inBar < 0.0 ? inBar + barLength : inBar) / barLength);

    // ------------------------------------------------------------ colours
    look::Palette target = look::manualPalette (lightTheme);
    float pulseTarget = 0.0f;

    if (beatMode)
    {
        const auto& set = look::beatPalettes (lightTheme);
        const int n = (int) set.size();

        // energetic music changes palette every bar, calmer music every 2 or 4 bars
        const float energy = juce::jlimit (0.0f, 1.0f, f.intensity + 0.4f * f.drop);
        const double bars = energy > 0.65f ? 1.0 : (energy > 0.3f ? 2.0 : 4.0);

        if (! initialised || beat < segmentStart || beat > segmentStart + segmentLength * 4.0)
        {
            segmentStart = std::floor (beat / barLength) * barLength;
            segmentLength = bars * barLength;
            initialised = true;
        }

        while (beat >= segmentStart + segmentLength)
        {
            segmentStart += segmentLength;
            segmentLength = bars * barLength;
            paletteIndex = (paletteIndex + 1) % n;
        }

        // crossfade into the next palette over the final two beats of the segment
        const float remaining = (float) (segmentStart + segmentLength - beat);
        const float blend = smoothstep (1.0f - remaining / 2.0f);
        target = look::mix (set[(size_t) paletteIndex], set[(size_t) ((paletteIndex + 1) % n)], blend);

        // slow hue drift through each bar keeps the colour alive between changes
        const float drift = 0.035f * std::sin (barPhaseValue * juce::MathConstants<float>::twoPi);
        target.a = target.a.withRotatedHue (drift);
        target.b = target.b.withRotatedHue (-drift);

        // a clear (not harsh) pulse on every beat - stronger on the downbeat and in louder passages
        const bool downbeat = inBar < 1.0;
        const float depth = (0.35f + 0.45f * energy) * (downbeat ? 1.3f : 1.0f) * (1.0f - 0.5f * f.breakdown);
        pulseTarget = juce::jlimit (0.0f, 1.0f, std::exp (-phase * 4.5f) * depth);
    }

    beatPulse = smoothTo (beatPulse, pulseTarget, dt, 0.01f, 0.06f);

    if (beatPulse > 0.001f)
    {
        const auto towards = lightTheme ? juce::Colours::black : juce::Colours::white;
        const auto accent = target.c;
        target.a = target.a.interpolatedWith (accent, beatPulse * 0.45f).interpolatedWith (towards, beatPulse * 0.2f);
        target.b = target.b.interpolatedWith (target.a, beatPulse * 0.25f);
    }

    // smooth every change (including switching MANUAL <-> BEAT COLOUR or the theme)
    const float k = 1.0f - std::exp (-dt / 0.05f);
    current = look::mix (current, target, k);
}
}

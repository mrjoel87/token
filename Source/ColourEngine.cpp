#include "ColourEngine.h"
#include "Util.h"
#include <cmath>

namespace token
{
void ColourEngine::update (float dt, const AudioFeatures& f, const Transport& t, bool beatMode, bool lightTheme)
{
    look::Palette target = look::manualPalette (lightTheme);
    float pulseTarget = 0.0f;

    // beat clock: the host's song position while playing, otherwise free-running at the host tempo
    const double bpm = juce::jlimit (20.0, 400.0, t.bpm);
    if (t.playing)
        freeBeat = t.ppq;
    else
        freeBeat += (double) dt * bpm / 60.0;

    const double beat = freeBeat;
    const double barLen = juce::jmax (1.0, t.beatsPerBar);
    phase = (float) (beat - std::floor (beat));

    if (beatMode)
    {
        const auto& set = look::beatPalettes (lightTheme);
        const int n = (int) set.size();

        // energetic music moves through colours faster; quiet music lingers
        const float energy = juce::jlimit (0.0f, 1.0f, f.intensity + 0.4f * f.drop);
        const double bars = energy > 0.72f ? 1.0 : (energy > 0.4f ? 2.0 : 4.0);

        if (! initialised || beat < segmentStart || beat > segmentStart + segmentLength * 4.0)
        {
            segmentStart = std::floor (beat / barLen) * barLen;
            segmentLength = bars * barLen;
            initialised = true;
        }

        while (beat >= segmentStart + segmentLength)
        {
            segmentStart += segmentLength;
            segmentLength = bars * barLen;
            paletteIndex = (paletteIndex + 1) % n;
        }

        // crossfade into the next palette across the last 40% of the segment
        const float progress = (float) ((beat - segmentStart) / segmentLength);
        const float blend = smoothstep ((progress - 0.6f) / 0.4f);
        target = look::mix (set[(size_t) paletteIndex], set[(size_t) ((paletteIndex + 1) % n)], blend);

        // gentle pulse on each beat, a little stronger on the downbeat and in louder passages
        const bool downbeat = std::fmod (beat, barLen) < 1.0;
        const float depth = (0.12f + 0.38f * energy) * (downbeat ? 1.25f : 1.0f) * (1.0f - 0.6f * f.breakdown);
        pulseTarget = std::exp (-phase * 5.0f) * depth;
    }

    beatPulse = smoothTo (beatPulse, pulseTarget, dt, 0.015f, 0.08f);

    // brighten (dark theme) or deepen (light theme) on the pulse - never a flash
    if (beatPulse > 0.001f)
    {
        const auto towards = lightTheme ? juce::Colours::black : juce::Colours::white;
        target.a = target.a.interpolatedWith (towards, beatPulse * 0.25f);
        target.b = target.b.interpolatedWith (towards, beatPulse * 0.15f);
    }

    // smooth every change (including switching MANUAL <-> BEAT COLOUR or the theme)
    const float k = 1.0f - std::exp (-dt / 0.12f);
    current = look::mix (current, target, k);
}
}

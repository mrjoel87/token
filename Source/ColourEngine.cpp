#include "ColourEngine.h"
#include "Util.h"
#include <cmath>

namespace token
{
void ColourEngine::update (float dt, const AudioFeatures& f, const Transport& t, int paletteIndex)
{
    // ------------------------------------------------------------ clock
    const double bpm = juce::jlimit (20.0, 400.0, t.bpm);
    barLength = juce::jmax (1.0, t.beatsPerBar);
    hostSynced = t.playing;

    if (t.playing)
        beat = t.ppq;
    else
        beat += (double) dt * bpm / 60.0;     // evolution only - not presented as a beat grid

    phase = (float) (beat - std::floor (beat));
    const double inBar = std::fmod (beat, barLength);
    barPhaseValue = (float) ((inBar < 0.0 ? inBar + barLength : inBar) / barLength);
    sinceSwitch += dt;

    // ------------------------------------------------------------ palette
    const int n = look::numStudioPalettes;
    look::Palette target;
    float pulseTarget = 0.0f;

    if (paletteIndex < n)
    {
        target = look::studioPalette (paletteIndex);
        pulseTarget = 0.25f * f.kick;                                   // a gentle kick bloom only
    }
    else
    {
        // PULSE: move through the palettes with the music
        if (hostSynced)
        {
            const double segment = barLength * (f.intensity > 0.55f ? 2.0 : 4.0);
            if (! initialised || beat < segmentStart || beat > segmentStart + segment * 4.0)
            {
                segmentStart = std::floor (beat / barLength) * barLength;
                initialised = true;
            }
            while (beat >= segmentStart + segment)
            {
                segmentStart += segment;
                pulseIndex = (pulseIndex + 1) % n;
            }
            const float remaining = (float) (segmentStart + segment - beat);
            const float blend = smoothstep (1.0f - remaining / 2.0f);
            target = look::mix (look::studioPalette (pulseIndex), look::studioPalette ((pulseIndex + 1) % n), blend);

            const bool downbeat = inBar < 1.0;
            pulseTarget = std::exp (-phase * 4.5f) * (0.35f + 0.45f * f.intensity) * (downbeat ? 1.3f : 1.0f);
        }
        else
        {
            initialised = false;
            if (f.kickHit && f.kick > 0.6f && sinceSwitch > 8.0f)
            {
                pulseIndex = (pulseIndex + 1) % n;
                sinceSwitch = 0.0f;
            }
            target = look::studioPalette (pulseIndex);
            pulseTarget = 0.6f * f.kick;
        }
    }

    pulseTarget *= 1.0f - 0.5f * f.breakdown;
    beatPulse = smoothTo (beatPulse, juce::jlimit (0.0f, 1.0f, pulseTarget), dt, 0.01f, 0.08f);

    if (beatPulse > 0.001f)
    {
        target.a = target.a.interpolatedWith (target.c, beatPulse * 0.35f).interpolatedWith (juce::Colours::white, beatPulse * 0.2f);
        target.b = target.b.interpolatedWith (target.a, beatPulse * 0.2f);
    }

    const float k = 1.0f - std::exp (-dt / 0.08f);
    current = look::mix (current, target, k);
}
}

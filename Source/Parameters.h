#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

namespace token
{
    // Parameter IDs. "mode", "clean", "intensity" (REACT), "artspeed" (SPEED), "lock", "palette" and
    // "theme" keep their IDs so settings saved in existing Live Sets still load.
    namespace ids
    {
        inline constexpr const char* mode    = "mode";        // layout: Strip / Box
        inline constexpr const char* clean   = "clean";       // Clean View (filming / projection)
        inline constexpr const char* theme   = "theme";       // Dark / Light
        inline constexpr const char* speed   = "artspeed";    // SPEED: rate of visual evolution
        inline constexpr const char* react   = "intensity";   // REACT: strength of audio response
        inline constexpr const char* palette = "palette";     // COLOUR
        inline constexpr const char* lock    = "lock";        // LOCK: freeze the composition
    }

    enum class DisplayMode { Strip = 0, Box = 1 };

    /** COLOUR choices: five fixed palettes, then PULSE (moves through them with the music). */
    inline constexpr int numPaletteChoices = 6;

    /** SPEED knob (0..1) -> evolution rate multiplier (0.3x .. 2.5x). */
    inline float speedFactor (float knob) noexcept
    {
        return 0.3f + 2.2f * std::pow (juce::jlimit (0.0f, 1.0f, knob), 1.3f);
    }

    namespace defaults
    {
        inline constexpr float react = 0.65f;
        inline constexpr float speed = 0.5f;

        inline constexpr int stripW = 1000, stripH = 200;
        inline constexpr int boxW   = 920,  boxH   = 640;
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        using namespace juce;
        AudioProcessorValueTreeState::ParameterLayout layout;

        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::mode, 1 }, "Layout",
                                                            StringArray { "Strip", "Box" }, 1));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::theme, 1 }, "Theme",
                                                            StringArray { "Dark", "Light" }, 0));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::palette, 1 }, "Colour",
                                                            StringArray { "Abyss", "Ultraviolet", "Acid", "Blood", "Gold", "Pulse" }, 0));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::speed, 1 }, "Speed",
                                                           NormalisableRange<float> (0.0f, 1.0f), defaults::speed));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::react, 1 }, "React",
                                                           NormalisableRange<float> (0.0f, 1.0f), defaults::react));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::lock, 1 }, "Lock", false));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::clean, 1 }, "Clean View", false));

        return layout;
    }
}

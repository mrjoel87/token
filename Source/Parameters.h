#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

namespace token
{
    // Parameter IDs. "mode", "clean", "intensity" (now REACT), "artspeed" (SPEED) and "lock" keep
    // their IDs so settings saved in existing Live Sets still load.
    namespace ids
    {
        inline constexpr const char* mode      = "mode";        // layout: Strip / Box
        inline constexpr const char* clean     = "clean";       // Clean View (filming)
        inline constexpr const char* world     = "world";       // SCENE: Sacred / Dream / Signal
        inline constexpr const char* speed     = "artspeed";    // SPEED: rate of visual evolution
        inline constexpr const char* react     = "intensity";   // REACT: strength of audio response
        inline constexpr const char* palette   = "palette";     // COLOUR
        inline constexpr const char* lock      = "lock";        // LOCK: freeze the composition
        inline constexpr const char* alignment = "alignment";   // ALIGNMENT moments on/off
    }

    enum class DisplayMode { Strip = 0, Box = 1 };
    enum class World       { Sacred = 0, Dream = 1, Signal = 2 };
    enum class PaletteId   { Abyss = 0, Ultraviolet, Amber, Ivory, Pulse };

    /** SPEED knob (0..1) -> evolution rate multiplier (0.3x .. 2.5x; default ~1.4x). */
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
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::world, 1 }, "Scene",
                                                            StringArray { "Sacred", "Dream", "Signal" }, 0));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::speed, 1 }, "Speed",
                                                           NormalisableRange<float> (0.0f, 1.0f), defaults::speed));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::react, 1 }, "React",
                                                           NormalisableRange<float> (0.0f, 1.0f), defaults::react));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::palette, 1 }, "Colour",
                                                            StringArray { "Abyss", "Ultraviolet", "Amber", "Ivory", "Pulse" }, 0));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::lock, 1 }, "Lock", false));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::alignment, 1 }, "Alignment", true));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::clean, 1 }, "Clean View", false));

        return layout;
    }
}

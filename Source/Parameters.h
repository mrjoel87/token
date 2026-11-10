#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

namespace token
{
    // Parameter IDs. "mode", "clean" and "theme" keep their original IDs so settings saved in
    // existing Live Sets still load.
    namespace ids
    {
        inline constexpr const char* mode      = "mode";       // layout: Strip / Box
        inline constexpr const char* clean     = "clean";      // Clean View
        inline constexpr const char* theme     = "theme";      // Dark / Light
        inline constexpr const char* visual    = "visual";     // Classic / Orbit / Mirror / None
        inline constexpr const char* colour    = "colour";     // Manual / Beat Colour
        inline constexpr const char* intensity = "intensity";  // strength of the visual reactions
        inline constexpr const char* artMode   = "artmode";    // Scope + Art / Art Only / Scope Only
        inline constexpr const char* speed     = "artspeed";   // how fast the visuals move and change
        inline constexpr const char* lock      = "lock";       // hold the current scene
    }

    enum class DisplayMode { Strip = 0, Box = 1 };
    enum class VisualMode  { Classic = 0, Orbit, Mirror, None };
    enum class ThemeId     { Dark = 0, Light = 1 };
    enum class ColourMode  { Manual = 0, Beat = 1 };
    enum class ArtMode     { ScopeAndArt = 0, ArtOnly = 1, ScopeOnly = 2 };

    /** SPEED knob (0..1) -> animation rate multiplier (0.3x .. 2.5x; default ~1.4x). */
    inline float speedFactor (float knob) noexcept
    {
        return 0.3f + 2.2f * std::pow (juce::jlimit (0.0f, 1.0f, knob), 1.3f);
    }

    namespace defaults
    {
        inline constexpr float intensity = 0.65f;
        inline constexpr float speed = 0.6f;

        inline constexpr int stripW = 1000, stripH = 220;
        inline constexpr int boxW   = 920,  boxH   = 640;
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        using namespace juce;
        AudioProcessorValueTreeState::ParameterLayout layout;

        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::mode, 1 }, "Layout",
                                                            StringArray { "Strip", "Box" }, 1));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::visual, 1 }, "Visual Mode",
                                                            StringArray { "Classic", "Orbit", "Mirror", "None" }, 0));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::theme, 1 }, "Theme",
                                                            StringArray { "Dark", "Light" }, 0));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::colour, 1 }, "Colour",
                                                            StringArray { "Manual", "Beat Colour" }, 0));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::intensity, 1 }, "Intensity",
                                                           NormalisableRange<float> (0.0f, 1.0f), defaults::intensity));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::clean, 1 }, "Clean View", false));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::artMode, 1 }, "Display",
                                                            StringArray { "Scope + Art", "Art Only", "Scope Only" }, 0));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::speed, 1 }, "Speed",
                                                           NormalisableRange<float> (0.0f, 1.0f), defaults::speed));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::lock, 1 }, "Lock Scene", false));

        return layout;
    }
}

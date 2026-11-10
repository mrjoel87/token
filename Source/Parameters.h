#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace token
{
    namespace ids
    {
        inline constexpr const char* mode     = "mode";
        inline constexpr const char* react    = "react";
        inline constexpr const char* chaos    = "chaos";
        inline constexpr const char* glow     = "glow";
        inline constexpr const char* scene    = "scene";
        inline constexpr const char* clean    = "clean";
        inline constexpr const char* display  = "display";
        inline constexpr const char* stereo   = "stereo";
        inline constexpr const char* autoGain = "autogain";
        inline constexpr const char* spectrum = "spectrum";
    }

    enum class DisplayMode { Strip = 0, Box = 1 };
    enum class SceneMode   { Auto = 0, Minimal, Dream, Mystic, Norse };

    namespace defaults
    {
        inline constexpr float react = 0.65f;
        inline constexpr float chaos = 0.45f;
        inline constexpr float glow  = 0.70f;

        inline constexpr int stripW = 1000, stripH = 220;
        inline constexpr int boxW   = 900,  boxH   = 650;
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        using namespace juce;
        AudioProcessorValueTreeState::ParameterLayout layout;

        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::mode, 1 }, "Mode",
                                                            StringArray { "Strip", "Box" }, 1));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::react, 1 }, "React",
                                                           NormalisableRange<float> (0.0f, 1.0f), defaults::react));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::chaos, 1 }, "Chaos",
                                                           NormalisableRange<float> (0.0f, 1.0f), defaults::chaos));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::glow, 1 }, "Glow",
                                                           NormalisableRange<float> (0.0f, 1.0f), defaults::glow));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::scene, 1 }, "Scene",
                                                            StringArray { "Auto", "Minimal", "Dream", "Mystic", "Norse" }, 0));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::clean, 1 }, "Clean Display", false));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::display, 1 }, "Scope Display",
                                                            StringArray { "Triggered", "Scrolling" }, 0));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::stereo, 1 }, "Stereo Split", false));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::autoGain, 1 }, "Auto Gain", true));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::spectrum, 1 }, "Spectrum", true));

        return layout;
    }
}

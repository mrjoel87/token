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

        // Hidden Transmission engine
        inline constexpr const char* theme      = "theme";
        inline constexpr const char* allImages  = "allimages";
        inline constexpr const char* catLost    = "catlost";
        inline constexpr const char* catOccult  = "catoccult";
        inline constexpr const char* catHuman   = "cathuman";
        inline constexpr const char* catNature  = "catnature";
        inline constexpr const char* catRoads   = "catroads";
        inline constexpr const char* sync       = "sync";
        inline constexpr const char* speed      = "speed";
        inline constexpr const char* density    = "density";
        inline constexpr const char* mystery    = "mystery";
        inline constexpr const char* visibility = "visibility";
        inline constexpr const char* rare       = "rare";
    }

    enum class DisplayMode { Strip = 0, Box = 1 };
    enum class SceneMode   { Auto = 0, Minimal, Dream, Mystic, Norse };
    enum class ThemeId     { Dark = 0, Light, Teal, Gold };
    enum class SyncMode    { Free = 0, Auto, Quarter, Half, One, Two, Four, Eight, Sixteen };
    enum class SpeedMode   { Slow = 0, Normal, Fast, Auto };

    namespace defaults
    {
        inline constexpr float react = 0.65f;
        inline constexpr float chaos = 0.45f;
        inline constexpr float glow  = 0.70f;
        inline constexpr float density    = 0.5f;
        inline constexpr float mystery    = 0.5f;
        inline constexpr float visibility = 0.6f;

        inline constexpr int stripW = 1000, stripH = 220;
        inline constexpr int boxW   = 920,  boxH   = 760;
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        using namespace juce;
        AudioProcessorValueTreeState::ParameterLayout layout;

        auto addFloat = [&layout] (const char* id, const char* name, float def)
        {
            layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 1.0f), def));
        };
        auto addBool = [&layout] (const char* id, const char* name, bool def)
        {
            layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
        };
        auto addChoice = [&layout] (const char* id, const char* name, const StringArray& choices, int def)
        {
            layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, choices, def));
        };

        addChoice (ids::mode, "Mode", StringArray { "Strip", "Box" }, 1);
        addFloat  (ids::react, "React", defaults::react);
        addFloat  (ids::chaos, "Chaos", defaults::chaos);
        addFloat  (ids::glow,  "Glow",  defaults::glow);
        addChoice (ids::scene, "Scene", StringArray { "Auto", "Minimal", "Dream", "Mystic", "Norse" }, 0);
        addBool   (ids::clean, "Clean Display", false);
        addChoice (ids::display, "Scope Display", StringArray { "Triggered", "Scrolling" }, 0);
        addBool   (ids::stereo, "Stereo Split", false);
        addBool   (ids::autoGain, "Auto Gain", true);
        addBool   (ids::spectrum, "Spectrum", true);

        addChoice (ids::theme, "Theme", StringArray { "Dark", "Light", "Teal", "Gold" }, 0);
        addBool   (ids::allImages, "All Images", true);
        addBool   (ids::catLost,   "Lost Worlds", true);
        addBool   (ids::catOccult, "Occult & Hidden", true);
        addBool   (ids::catHuman,  "Human Moments", true);
        addBool   (ids::catNature, "Nature", true);
        addBool   (ids::catRoads,  "Roads & Cities", true);
        addChoice (ids::sync, "Image Sync", StringArray { "Free", "Auto", "1/4 Bar", "1/2 Bar", "1 Bar", "2 Bars", "4 Bars", "8 Bars", "16 Bars" }, 1);
        addChoice (ids::speed, "Image Speed", StringArray { "Slow", "Normal", "Fast", "Auto" }, 3);
        addFloat  (ids::density,    "Image Density",    defaults::density);
        addFloat  (ids::mystery,    "Mystery",          defaults::mystery);
        addFloat  (ids::visibility, "Image Visibility", defaults::visibility);
        addBool   (ids::rare, "Rare Apparitions", true);

        return layout;
    }
}

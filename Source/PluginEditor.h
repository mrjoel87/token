#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "AudioAnalyser.h"
#include "ColourEngine.h"
#include "Visualiser.h"
#include "Art/ArtEngine.h"
#include "Theme.h"
#include "Transport.h"
#include "UI/TotemLookAndFeel.h"
#include "UI/Controls.h"

namespace token
{
class TokenEditor final : public juce::AudioProcessorEditor
{
public:
    explicit TokenEditor (TokenAudioProcessor&);
    ~TokenEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    struct Caption
    {
        juce::String text;
        juce::Rectangle<float> area;
    };

    // frame loop
    void frame();
    Transport readTransport (double now) const;
    bool lightTheme() const;

    // layout
    void applyMode (DisplayMode newMode, bool force);
    void updateVisibility();
    void layoutStrip();
    void layoutBox();
    void layoutClean();
    juce::Rectangle<float> visualArea() const;
    juce::Rectangle<float> artCanvas() const;
    void placeRow (juce::Rectangle<float> row, std::initializer_list<std::pair<juce::Component*, const char*>> items,
                   std::initializer_list<float> weights, bool captionsAbove);

    // painting
    void paintScreen (juce::Graphics&, const look::Look&);
    void paintWordmark (juce::Graphics&, const look::Look&, juce::Rectangle<float>, bool compact);
    void paintInfo (juce::Graphics&, const look::Look&);

    TokenAudioProcessor& proc;
    TotemLookAndFeel lookAndFeel;

    SegmentSelector visualSelector, colourSelector, themeSelector, layoutSelector;
    LedToggle cleanToggle;
    SegmentSelector displaySelector;
    LedToggle lockToggle;
    juce::Slider intensitySlider, speedSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> intensityAttachment, speedAttachment;

    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pVisual = nullptr;
    std::atomic<float>* pTheme = nullptr;
    std::atomic<float>* pColour = nullptr;
    std::atomic<float>* pIntensity = nullptr;
    std::atomic<float>* pClean = nullptr;
    std::atomic<float>* pArt = nullptr;
    std::atomic<float>* pSpeed = nullptr;
    std::atomic<float>* pLock = nullptr;

    DisplayMode mode = DisplayMode::Box;
    bool clean = false, applyingMode = false, hasFrame = false;

    juce::Rectangle<float> screenRect, brandRect, barRect;
    std::vector<Caption> captions;

    ScopeHistory history;
    std::vector<float> pullLeft, pullRight;
    AudioAnalyser analyser;
    Visualiser visualiser;
    ColourEngine colours;
    art::ArtEngine artEngine;
    art::ArtContext artContext;
    ArtMode artMode = ArtMode::ScopeAndArt;
    float scopeFade = 1.0f;
    VisualSettings visualSettings;
    Transport transport;
    float time = 0.0f;
    juce::Random fxRandom;

    juce::Image grainTexture;
    double startTime = 0.0, lastTime = 0.0;
    std::unique_ptr<juce::VBlankAttachment> vblank;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TokenEditor)
};
}

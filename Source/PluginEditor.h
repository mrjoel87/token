#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "AudioAnalyser.h"
#include "Oscilloscope.h"
#include "World/WorldEngine.h"
#include "UI/TokenLookAndFeel.h"
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
    struct EngravedLabel
    {
        juce::String text;
        juce::Rectangle<float> area;
        float height;
        juce::Justification justification;
    };

    // frame loop
    void frame();
    ScopeSettings readScopeSettings() const;

    // layout
    void applyMode (DisplayMode newMode, bool force);
    void updateVisibility();
    void layoutStrip();
    void layoutBox();
    void layoutClean();
    void placeKnobs (juce::Rectangle<float> area, float knobSize, float labelHeight);
    juce::Rectangle<float> scopeArea() const;
    juce::Rectangle<float> spectrumArea() const;

    // painting
    void paintChassis (juce::Graphics&);
    void paintScreen (juce::Graphics&);
    void paintScreenOverlay (juce::Graphics&);
    void paintHud (juce::Graphics&);
    void paintPanel (juce::Graphics&);
    void paintBrand (juce::Graphics&);

    TokenAudioProcessor& proc;
    TokenLookAndFeel lookAndFeel;

    juce::Slider reactKnob, chaosKnob, glowKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reactAttachment, chaosAttachment, glowAttachment;
    SegmentSelector modeSelector, sceneSelector, displaySelector;
    LedToggle stereoToggle, gainToggle, spectrumToggle, cleanToggle;

    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pReact = nullptr;
    std::atomic<float>* pChaos = nullptr;
    std::atomic<float>* pGlow = nullptr;
    std::atomic<float>* pScene = nullptr;
    std::atomic<float>* pClean = nullptr;
    std::atomic<float>* pDisplay = nullptr;
    std::atomic<float>* pStereo = nullptr;
    std::atomic<float>* pAutoGain = nullptr;
    std::atomic<float>* pSpectrum = nullptr;

    DisplayMode mode = DisplayMode::Box;
    bool clean = false, applyingMode = false, hasFrame = false;

    juce::Rectangle<float> screenRect, panelRect, brandRect;
    std::vector<EngravedLabel> labels;

    ScopeHistory history;
    std::vector<float> pullLeft, pullRight;
    AudioAnalyser analyser;
    Oscilloscope scope;
    WorldEngine world;
    WorldContext ctx;
    ScopeSettings scopeSettings;
    SceneMode sceneMode = SceneMode::Auto;
    juce::Random fxRandom;

    juce::Image brushedTexture, grainTexture, scanTexture;
    double startTime = 0.0, lastTime = 0.0;
    std::unique_ptr<juce::VBlankAttachment> vblank;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TokenEditor)
};
}

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "AudioAnalyser.h"
#include "ColourEngine.h"
#include "Engine/TotemEngine.h"
#include "Theme.h"
#include "Transport.h"
#include "UI/TotemLookAndFeel.h"
#include "UI/Controls.h"

namespace token
{
/**
    SMSW TOTEM 2.0 editor.

    Performance rules (see README "Performance"):
      - every vblank only the screen rectangle is repainted; the chassis and controls repaint only
        when something about them changes
      - no Path clip regions, no per-element transparency layers, cached vignette image
      - the engine's drawing time is measured and the quality level (2 / 1 / 0) adapts to it
*/
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
    void adaptQuality (double paintMs);

    // layout
    void applyMode (DisplayMode newMode, bool force);
    void updateVisibility();
    void layoutStrip();
    void layoutBox();
    void layoutClean();
    juce::Rectangle<float> visualArea() const;
    void placeRow (juce::Rectangle<float> row, std::initializer_list<std::pair<juce::Component*, const char*>> items,
                   std::initializer_list<float> weights, bool captionsAbove);
    void rebuildScreenCache();

    // painting
    void paintChrome (juce::Graphics&, const look::Look&);
    void paintScreen (juce::Graphics&, const look::Look&);
    void paintWordmark (juce::Graphics&, const look::Look&, juce::Rectangle<float>, bool compact);
    void paintHud (juce::Graphics&, const look::Look&);

    TokenAudioProcessor& proc;
    TotemLookAndFeel lookAndFeel;

    SegmentSelector sceneSelector, colourSelector, layoutSelector;
    LedToggle lockToggle, alignToggle, cleanToggle;
    juce::Slider reactSlider, speedSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reactAttachment, speedAttachment;

    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pClean = nullptr;
    std::atomic<float>* pWorld = nullptr;
    std::atomic<float>* pPalette = nullptr;
    std::atomic<float>* pReact = nullptr;
    std::atomic<float>* pSpeed = nullptr;
    std::atomic<float>* pLock = nullptr;
    std::atomic<float>* pAlign = nullptr;

    DisplayMode mode = DisplayMode::Box;
    bool clean = false, applyingMode = false, hasFrame = false;

    juce::Rectangle<float> screenRect, brandRect, barRect;
    std::vector<Caption> captions;
    juce::Path screenCorners;        // chassis-coloured corner masks (instead of a rounded clip)
    juce::Image vignette;            // cached once per size

    ScopeHistory history;
    std::vector<float> pullLeft, pullRight;
    AudioAnalyser analyser;
    ColourEngine colours;
    engine::TotemEngine totem;
    engine::EngineSettings settings;
    Transport transport;
    float time = 0.0f;

    int quality = 2, lastPalette = -1;
    double paintAvgMs = 0.0, goodTime = 0.0;
    juce::Colour lastAccent;

    double startTime = 0.0, lastTime = 0.0;
    std::unique_ptr<juce::VBlankAttachment> vblank;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TokenEditor)
};
}

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace token
{
/** Compact pill selector with a clear selected state, bound to a choice parameter. */
class SegmentSelector final : public juce::Component
{
public:
    SegmentSelector (juce::RangedAudioParameter& parameter, juce::StringArray segmentLabels);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    int indexAt (float x) const noexcept;

    juce::StringArray labels;
    int selected = 0, hover = -1;
    juce::ParameterAttachment attachment;
};

/** Small on/off pill bound to a bool parameter. In "ghost" mode it is invisible until hovered. */
class LedToggle final : public juce::Component
{
public:
    LedToggle (juce::RangedAudioParameter& parameter, juce::String labelText);

    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { repaint(); }

    void setGhost (bool shouldBeGhost) { ghost = shouldBeGhost; repaint(); }

private:
    juce::String label;
    bool on = false, ghost = false;
    juce::ParameterAttachment attachment;
};
}

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace token
{
namespace ui
{
    /** Bahnschrift (ships with Windows 10/11) - weight: "Light", "SemiLight", "Regular", "SemiBold". */
    juce::Font font (float height, const juce::String& weight = "Regular", float kerning = 0.12f);
}

class TotemLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle, juce::Slider&) override;

    void drawCornerResizer (juce::Graphics&, int w, int h, bool isMouseOver, bool isMouseDragging) override;
};
}

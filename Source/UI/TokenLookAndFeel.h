#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace token
{
namespace ui
{
    /** Bahnschrift (ships with Windows 10/11) with wide tracking: engraved-hardware typography. */
    juce::Font font (float height, bool bold = false, float kerning = 0.16f);

    /** Text cut into metal: dark shadow above, faint highlight below. */
    void engrave (juce::Graphics&, const juce::String& text, juce::Rectangle<float> area, float height,
                  juce::Colour colour, juce::Justification justification, bool bold = false);
}

class TokenLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    TokenLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

    void drawCornerResizer (juce::Graphics&, int w, int h, bool isMouseOver, bool isMouseDragging) override;
};
}

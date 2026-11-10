#include "TotemLookAndFeel.h"
#include "../Theme.h"

namespace token
{
namespace ui
{
    juce::Font font (float height, const juce::String& weight, float kerning)
    {
        return juce::Font (juce::FontOptions ("Bahnschrift", weight, height).withKerningFactor (kerning));
    }
}

void TotemLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                         float, float, juce::Slider::SliderStyle, juce::Slider& slider)
{
    const auto& lk = look::active();
    const auto accent = lk.light ? lk.pillOn : look::accent();
    const float cy = (float) y + (float) height * 0.5f;
    const float x0 = (float) x + 6.0f, x1 = (float) (x + width) - 6.0f;
    const float pos = juce::jlimit (x0, x1, sliderPos);

    // track
    g.setColour (lk.track);
    g.fillRoundedRectangle (x0, cy - 1.0f, x1 - x0, 2.0f, 1.0f);

    // filled part
    g.setColour (accent);
    g.fillRoundedRectangle (x0, cy - 1.0f, pos - x0, 2.0f, 1.0f);

    // fine ticks
    g.setColour (lk.textDim.withAlpha (0.35f));
    for (int i = 0; i <= 10; ++i)
    {
        const float tx = x0 + (x1 - x0) * (float) i / 10.0f;
        g.fillRect (tx - 0.5f, cy + 4.0f, 1.0f, i % 5 == 0 ? 3.0f : 1.5f);
    }

    // thumb
    const float r = slider.isMouseOverOrDragging() ? 6.0f : 5.0f;
    if (! lk.light)
    {
        g.setColour (accent.withAlpha (0.18f));
        g.fillEllipse (pos - r * 2.0f, cy - r * 2.0f, r * 4.0f, r * 4.0f);
    }
    g.setColour (lk.light ? lk.pillOn : lk.text);
    g.fillEllipse (pos - r, cy - r, r * 2.0f, r * 2.0f);
    g.setColour (lk.light ? lk.screenTop : lk.chassisBottom);
    g.fillEllipse (pos - r * 0.35f, cy - r * 0.35f, r * 0.7f, r * 0.7f);
}

void TotemLookAndFeel::drawCornerResizer (juce::Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging)
{
    const auto& lk = look::active();
    g.setColour (lk.textDim.withAlpha ((isMouseOver || isMouseDragging) ? 0.7f : 0.3f));

    for (int i = 1; i <= 3; ++i)
    {
        const float d = (float) i * (float) juce::jmin (w, h) / 4.0f;
        g.fillEllipse ((float) w - d - 1.0f, (float) h - 2.5f, 1.6f, 1.6f);
        g.fillEllipse ((float) w - 2.5f, (float) h - d - 1.0f, 1.6f, 1.6f);
    }
}
}

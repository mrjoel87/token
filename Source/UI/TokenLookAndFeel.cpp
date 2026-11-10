#include "TokenLookAndFeel.h"
#include "../Palette.h"
#include <cmath>

namespace token
{
namespace ui
{
    juce::Font font (float height, bool bold, float kerning)
    {
        return juce::Font (juce::FontOptions ("Bahnschrift", height, bold ? juce::Font::bold : juce::Font::plain)
                               .withKerningFactor (kerning));
    }

    void engrave (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, float height,
                  juce::Colour colour, juce::Justification justification, bool bold)
    {
        g.setFont (font (height, bold));
        g.setColour (juce::Colours::black.withAlpha (0.75f));
        g.drawText (text, area.translated (0.0f, -1.0f), justification, false);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawText (text, area.translated (0.0f, 1.0f), justification, false);
        g.setColour (colour);
        g.drawText (text, area, justification, false);
    }
}

TokenLookAndFeel::TokenLookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId, pal::cyan);
    setColour (juce::Slider::thumbColourId, pal::gold);
}

void TokenLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                                         float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (2.0f);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto c = bounds.getCentre();
    const float R = size * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);
    const bool hover = slider.isMouseOverOrDragging();

    // engraved scale ticks
    for (int i = 0; i <= 10; ++i)
    {
        const float t = (float) i / 10.0f;
        const float a = startAngle + t * (endAngle - startAngle);
        const bool major = i % 5 == 0;
        const auto p0 = c.getPointOnCircumference (R * (major ? 0.84f : 0.88f), a);
        const auto p1 = c.getPointOnCircumference (R * 0.97f, a);
        const bool lit = t <= pos + 1.0e-3f;
        g.setColour (lit ? pal::cyan.withAlpha (0.75f) : pal::gold.withAlpha (0.3f));
        g.drawLine ({ p0, p1 }, major ? 1.3f : 0.9f);
    }

    // value arc: groove + luminous fill
    const float arcR = R * 0.76f;
    juce::Path groove;
    groove.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.strokePath (groove, juce::PathStrokeType (R * 0.07f + 1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (pos > 0.001f)
    {
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
        const float widths[] = { R * 0.30f, R * 0.16f, R * 0.06f };
        const float alphas[] = { 0.07f, 0.16f, 0.9f };
        for (int i = 0; i < 3; ++i)
        {
            g.setColour (pal::cyan.withAlpha (alphas[i] * (hover ? 1.0f : 0.85f)));
            g.strokePath (arc, juce::PathStrokeType (widths[i], juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    // knob body: shadow, knurled titanium ring, brushed face, gold bezel
    const float kr = R * 0.6f;
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (c.x - kr - 1.0f, c.y - kr + 1.5f, (kr + 1.0f) * 2.0f, (kr + 1.0f) * 2.0f);

    juce::ColourGradient ring (juce::Colour (0xff5d636d), c.x - kr * 0.6f, c.y - kr * 0.8f,
                               juce::Colour (0xff121417), c.x + kr * 0.6f, c.y + kr, false);
    g.setGradientFill (ring);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    for (int i = 0; i < 40; ++i)
    {
        const float a = angle + (float) i / 40.0f * juce::MathConstants<float>::twoPi;
        g.drawLine ({ c.getPointOnCircumference (kr * 0.84f, a), c.getPointOnCircumference (kr * 0.99f, a) }, 0.8f);
    }

    const float fr = kr * 0.78f;
    juce::ColourGradient face (juce::Colour (0xff3f444c), c.x, c.y - fr, juce::Colour (0xff1a1d22), c.x, c.y + fr, false);
    g.setGradientFill (face);
    g.fillEllipse (c.x - fr, c.y - fr, fr * 2.0f, fr * 2.0f);

    g.setColour (juce::Colours::white.withAlpha (0.035f));
    for (float rr = fr * 0.25f; rr < fr; rr += juce::jmax (1.5f, fr * 0.12f))
        g.drawEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f, 0.6f);

    g.setColour (pal::gold.withAlpha (hover ? 0.75f : 0.5f));
    g.drawEllipse (c.x - fr, c.y - fr, fr * 2.0f, fr * 2.0f, 0.8f);

    // pointer
    const auto p0 = c.getPointOnCircumference (fr * 0.25f, angle);
    const auto p1 = c.getPointOnCircumference (fr * 0.92f, angle);
    g.setColour (pal::goldBright.withAlpha (0.25f));
    g.drawLine ({ p0, p1 }, 4.0f);
    g.setColour (pal::goldBright);
    g.drawLine ({ p0, p1 }, 1.6f);
    g.fillEllipse (c.x - 1.5f, c.y - 1.5f, 3.0f, 3.0f);
}

void TokenLookAndFeel::drawCornerResizer (juce::Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging)
{
    const float alpha = (isMouseOver || isMouseDragging) ? 0.6f : 0.25f;
    g.setColour (pal::gold.withAlpha (alpha));

    for (int i = 1; i <= 3; ++i)
    {
        const float d = (float) i * (float) juce::jmin (w, h) / 4.0f;
        g.fillEllipse ((float) w - d - 1.0f, (float) h - 2.5f, 1.6f, 1.6f);
        g.fillEllipse ((float) w - 2.5f, (float) h - d - 1.0f, 1.6f, 1.6f);
    }
}
}

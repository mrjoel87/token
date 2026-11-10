#include "Controls.h"
#include "TokenLookAndFeel.h"
#include "../Palette.h"

namespace token
{
// ================================================================================================= SegmentSelector
SegmentSelector::SegmentSelector (juce::RangedAudioParameter& parameter, juce::StringArray segmentLabels)
    : labels (std::move (segmentLabels)),
      attachment (parameter,
                  [this] (float value)
                  {
                      selected = juce::jlimit (0, juce::jmax (0, labels.size() - 1), juce::roundToInt (value));
                      repaint();
                  },
                  nullptr)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    attachment.sendInitialUpdate();
}

int SegmentSelector::indexAt (float x) const noexcept
{
    const int n = juce::jmax (1, labels.size());
    return juce::jlimit (0, n - 1, (int) (x / (float) juce::jmax (1, getWidth()) * (float) n));
}

void SegmentSelector::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (0.5f);
    const int n = juce::jmax (1, labels.size());
    const float segW = b.getWidth() / (float) n;
    const float textH = juce::jlimit (7.0f, 11.0f, b.getHeight() * 0.56f);

    // recess
    g.setColour (pal::lightScreen ? juce::Colour (0xffc4c5c1) : juce::Colour (0xff0a0b0e));
    g.fillRoundedRectangle (b, 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (b, 3.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawHorizontalLine ((int) b.getBottom(), b.getX() + 3.0f, b.getRight() - 3.0f);

    for (int i = 0; i < n; ++i)
    {
        const auto r = juce::Rectangle<float> (b.getX() + segW * (float) i, b.getY(), segW, b.getHeight());
        const bool isSel = i == selected;

        if (i > 0)
        {
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.fillRect (r.getX() - 0.5f, r.getY() + 3.0f, 1.0f, r.getHeight() - 6.0f);
        }

        if (isSel)
        {
            const auto inner = r.reduced (1.5f);
            juce::ColourGradient grad (pal::cyan.withAlpha (0.16f), 0.0f, inner.getY(), pal::cyan.withAlpha (0.04f), 0.0f, inner.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (inner, 2.0f);
            g.setColour (pal::cyan.withAlpha (0.35f));
            g.drawRoundedRectangle (inner, 2.0f, 0.7f);
        }

        g.setFont (ui::font (textH, isSel, 0.12f));
        const auto colour = isSel ? pal::cyan.interpolatedWith (pal::core, 0.3f)
                                  : pal::gold.withAlpha (i == hover ? 0.85f : 0.45f);
        g.setColour (colour);
        g.drawText (labels[i], r, juce::Justification::centred, false);
    }
}

void SegmentSelector::mouseDown (const juce::MouseEvent& e)
{
    const int idx = indexAt ((float) e.x);
    if (idx != selected)
        attachment.setValueAsCompleteGesture ((float) idx);
}

void SegmentSelector::mouseMove (const juce::MouseEvent& e)
{
    const int idx = indexAt ((float) e.x);
    if (idx != hover) { hover = idx; repaint(); }
}

void SegmentSelector::mouseExit (const juce::MouseEvent&)
{
    hover = -1;
    repaint();
}

// ================================================================================================= LedToggle
LedToggle::LedToggle (juce::RangedAudioParameter& parameter, juce::String labelText)
    : label (std::move (labelText)),
      attachment (parameter, [this] (float value) { on = value > 0.5f; repaint(); }, nullptr)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    attachment.sendInitialUpdate();
}

void LedToggle::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const bool over = isMouseOver (true);

    if (ghost && ! over)
        return;

    const float d = juce::jlimit (4.0f, 8.0f, b.getHeight() * 0.42f);
    const auto led = juce::Rectangle<float> (d, d).withCentre ({ ghost ? b.getCentreX() : b.getX() + d * 0.5f + 2.0f, b.getCentreY() });

    if (on)
    {
        g.setColour (pal::cyan.withAlpha (0.25f));
        g.fillEllipse (led.expanded (d * 0.6f));
        g.setColour (pal::cyan.interpolatedWith (pal::core, 0.35f));
    }
    else
    {
        g.setColour (juce::Colour (0xff2a1a12));
    }
    g.fillEllipse (led);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawEllipse (led, 0.8f);

    if (! ghost)
    {
        const auto textArea = b.withTrimmedLeft (d + 6.0f);
        g.setFont (ui::font (juce::jlimit (7.0f, 10.0f, b.getHeight() * 0.52f), false, 0.12f));
        g.setColour (pal::gold.withAlpha (over ? 0.9f : 0.6f));
        g.drawText (label, textArea, juce::Justification::centredLeft, false);
    }
}

void LedToggle::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked() && getLocalBounds().contains (e.getPosition()))
        attachment.setValueAsCompleteGesture (on ? 0.0f : 1.0f);
}
}

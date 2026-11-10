#include "Controls.h"
#include "TotemLookAndFeel.h"
#include "../Theme.h"

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
    const auto& lk = look::active();
    const auto b = getLocalBounds().toFloat().reduced (0.5f);
    const int n = juce::jmax (1, labels.size());
    const float segW = b.getWidth() / (float) n;
    const float radius = b.getHeight() * 0.5f;
    const float textH = juce::jlimit (8.0f, 11.5f, b.getHeight() * 0.48f);

    g.setColour (lk.pillOff);
    g.fillRoundedRectangle (b, radius);
    g.setColour (lk.pillEdge);
    g.drawRoundedRectangle (b, radius, 1.0f);

    for (int i = 0; i < n; ++i)
    {
        const auto r = juce::Rectangle<float> (b.getX() + segW * (float) i, b.getY(), segW, b.getHeight());
        const bool isSel = i == selected;

        if (isSel)
        {
            const auto inner = r.reduced (2.0f);
            g.setColour (lk.pillOn);
            g.fillRoundedRectangle (inner, inner.getHeight() * 0.5f);
            if (! lk.light)
            {
                g.setColour (look::accent().withAlpha (0.55f));
                g.drawRoundedRectangle (inner, inner.getHeight() * 0.5f, 0.8f);
            }
        }

        g.setFont (ui::font (textH, isSel ? "SemiBold" : "Regular", 0.14f));
        g.setColour (isSel ? lk.pillOnText : (i == hover ? lk.text : lk.pillOffText));
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
    const auto& lk = look::active();
    const auto b = getLocalBounds().toFloat().reduced (0.5f);
    const bool over = isMouseOver (true);

    if (ghost)
    {
        if (! over)
            return;
        g.setColour (lk.pillOff.withAlpha (0.85f));
        g.fillEllipse (b);
        g.setColour (lk.text);
        g.drawEllipse (b.reduced (4.0f), 1.0f);
        return;
    }

    const float radius = b.getHeight() * 0.5f;
    g.setColour (on ? lk.pillOn : lk.pillOff);
    g.fillRoundedRectangle (b, radius);
    g.setColour (on && ! lk.light ? look::accent().withAlpha (0.55f) : lk.pillEdge);
    g.drawRoundedRectangle (b, radius, on && ! lk.light ? 0.8f : 1.0f);

    const float d = juce::jlimit (4.0f, 7.0f, b.getHeight() * 0.3f);
    const auto dot = juce::Rectangle<float> (d, d).withCentre ({ b.getX() + radius + 2.0f, b.getCentreY() });
    if (on)
    {
        g.setColour (lk.light ? lk.pillOnText : look::accent());
        g.fillEllipse (dot);
    }
    else
    {
        g.setColour (lk.pillOffText);
        g.drawEllipse (dot, 1.0f);
    }

    g.setFont (ui::font (juce::jlimit (8.0f, 11.0f, b.getHeight() * 0.46f), on ? "SemiBold" : "Regular", 0.14f));
    g.setColour (on ? lk.pillOnText : (over ? lk.text : lk.pillOffText));
    g.drawText (label, b.withTrimmedLeft (radius + d + 2.0f).withTrimmedRight (radius * 0.5f), juce::Justification::centred, false);
}

void LedToggle::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked() && getLocalBounds().contains (e.getPosition()))
        attachment.setValueAsCompleteGesture (on ? 0.0f : 1.0f);
}
}

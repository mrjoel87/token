#pragma once

#include <juce_graphics/juce_graphics.h>

namespace token::pixel
{
    using Palette = juce::Colour (*) (char);

    /** Draws a character-grid sprite ('.' or ' ' = transparent) with seam-free integer-snapped pixels. */
    void drawSprite (juce::Graphics&, const char* const* rows, int numRows, float x, float y, float px,
                     Palette palette, float alpha, bool flipX = false);

    /** Multi-pass luminous stroke used for runes, glyphs and outlines. */
    void drawGlowPath (juce::Graphics&, const juce::Path&, juce::Colour, float intensity, float thickness);

    /** Polyline glyph from "x,y x,y|x,y ..." in percent of a unit box (0..1). */
    juce::Path unitPath (const char* spec);

    // ---- Elder Futhark (historical runes only - no modern fantasy symbols)
    int numRunes() noexcept;
    int findRune (const char* name) noexcept;
    const char* runeName (int index) noexcept;
    juce::Path runePath (int index, juce::Rectangle<float> box);
    void drawRune (juce::Graphics&, int index, juce::Rectangle<float> box, juce::Colour, float intensity, float thickness);
}

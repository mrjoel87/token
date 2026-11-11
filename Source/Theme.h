#pragma once

#include <juce_graphics/juce_graphics.h>
#include <array>

/*
    SMSW TOTEM visual identity: two themes (Dark / Light) and the colour palettes the
    visualisation is painted with. Interface colours come from Look; the artwork's colours come
    from Palette (fixed in MANUAL, evolving in BEAT COLOUR).
*/
namespace token::look
{
    /** The three colours of the artwork: line, glow / secondary line, accent detail. */
    struct Palette
    {
        juce::Colour a, b, c;
    };

    struct Look
    {
        bool light = false;
        juce::Colour chassisTop, chassisBottom;     // window background
        juce::Colour screenTop, screenBottom;       // visualisation surface
        juce::Colour grid, frame;                   // hairlines
        juce::Colour separator;
        juce::Colour text, textDim, caption;        // typography
        juce::Colour pillOff, pillOffText, pillEdge;
        juce::Colour pillOn, pillOnText;
        juce::Colour track;                         // slider track
    };

    const Look& dark();
    const Look& light();

    /** The look used by the controls while they paint (set by the editor before painting). */
    void setActive (bool lightTheme) noexcept;
    const Look& active() noexcept;

    /** Accent for selected states / slider fill, kept in step with the artwork's palette. */
    void setAccent (juce::Colour) noexcept;
    juce::Colour accent() noexcept;

    /** TOTEM 2.0 colour palettes: 0 Abyss, 1 Ultraviolet, 2 Amber, 3 Ivory. */
    const Palette& studioPalette (int index);
    constexpr int numStudioPalettes = 4;

    Palette manualPalette (bool lightTheme);
    const std::array<Palette, 6>& beatPalettes (bool lightTheme);
    Palette mix (const Palette& from, const Palette& to, float t);
}

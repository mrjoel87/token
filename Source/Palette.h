#pragma once

#include <juce_graphics/juce_graphics.h>
#include <array>

namespace token::pal
{
    // Primary palette
    inline const juce::Colour black      { 0xff030407 };
    inline const juce::Colour midnight   { 0xff0a1430 };
    inline const juce::Colour cyan       { 0xff46eeff };
    inline const juce::Colour turquoise  { 0xff5fd8c4 };
    inline const juce::Colour gold       { 0xffc9a45c };
    inline const juce::Colour goldBright { 0xfff2d79a };
    inline const juce::Colour goldDark   { 0xff6a5226 };

    // Accent palette
    inline const juce::Colour amber      { 0xffffb35a };
    inline const juce::Colour violet     { 0xff6a48b8 };
    inline const juce::Colour pink       { 0xffffb6cf };
    inline const juce::Colour crimson    { 0xffc8283c };
    inline const juce::Colour moon       { 0xffe8f0ff };

    /** Weighted blend of three colours (weights need not sum to one). */
    inline juce::Colour mix3 (const std::array<float, 3>& w, juce::Colour a, juce::Colour b, juce::Colour c)
    {
        const float sum = juce::jmax (1.0e-4f, w[0] + w[1] + w[2]);
        auto ch = [&] (float x, float y, float z) { return (x * w[0] + y * w[1] + z * w[2]) / sum; };

        return juce::Colour::fromFloatRGBA (ch (a.getFloatRed(),   b.getFloatRed(),   c.getFloatRed()),
                                            ch (a.getFloatGreen(), b.getFloatGreen(), c.getFloatGreen()),
                                            ch (a.getFloatBlue(),  b.getFloatBlue(),  c.getFloatBlue()),
                                            ch (a.getFloatAlpha(), b.getFloatAlpha(), c.getFloatAlpha()));
    }
}

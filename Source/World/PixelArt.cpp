#include "PixelArt.h"
#include <cstring>
#include <vector>

namespace token::pixel
{
void drawSprite (juce::Graphics& g, const char* const* rows, int numRows, float x, float y, float px,
                 Palette palette, float alpha, bool flipX)
{
    if (alpha <= 0.003f)
        return;

    for (int r = 0; r < numRows; ++r)
    {
        const char* row = rows[r];
        const int len = (int) std::strlen (row);
        const int y0 = juce::roundToInt (y + (float) r * px);
        const int y1 = juce::roundToInt (y + (float) (r + 1) * px);
        int c = 0;

        while (c < len)
        {
            const char ch = row[c];
            int end = c + 1;
            while (end < len && row[end] == ch)
                ++end;                                  // merge horizontal runs into one rectangle

            if (ch != '.' && ch != ' ')
            {
                const auto colour = palette (ch);

                if (! colour.isTransparent())
                {
                    const int a = flipX ? len - end : c;
                    const int b = flipX ? len - c : end;
                    const int x0 = juce::roundToInt (x + (float) a * px);
                    const int x1 = juce::roundToInt (x + (float) b * px);
                    g.setColour (colour.withMultipliedAlpha (alpha));
                    g.fillRect (x0, y0, x1 - x0, y1 - y0);
                }
            }

            c = end;
        }
    }
}

void drawGlowPath (juce::Graphics& g, const juce::Path& p, juce::Colour c, float intensity, float thickness)
{
    if (intensity <= 0.003f)
        return;

    static constexpr float widths[] = { 7.0f, 3.6f, 1.9f };
    static constexpr float alphas[] = { 0.07f, 0.16f, 0.38f };

    for (int i = 0; i < 3; ++i)
    {
        g.setColour (c.withAlpha (juce::jlimit (0.0f, 1.0f, alphas[i] * intensity)));
        g.strokePath (p, juce::PathStrokeType (thickness * widths[i], juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour (c.interpolatedWith (juce::Colours::white, 0.5f).withAlpha (juce::jlimit (0.0f, 1.0f, 0.95f * intensity)));
    g.strokePath (p, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// ------------------------------------------------------------------------------------------------
// Elder Futhark letterforms, simplified to straight staves and branches (as carved in stone/wood).
// Coordinates are percentages of a box: "x,y x,y | next stroke".
// ------------------------------------------------------------------------------------------------
namespace
{
    struct RuneSpec { const char* name; const char* strokes; };

    const RuneSpec runeSpecs[] = {
        { "fehu",     "30,100 30,0|30,40 75,10|30,65 75,35" },
        { "uruz",     "25,100 25,0 75,35 75,100" },
        { "thurisaz", "30,0 30,100|30,25 72,50 30,75" },
        { "ansuz",    "30,100 30,0|30,0 72,28|30,30 72,58" },
        { "raidho",   "30,100 30,0 72,25 30,50 72,100" },
        { "kaunan",   "70,15 30,50 70,85" },
        { "gebo",     "20,0 80,100|80,0 20,100" },
        { "wunjo",    "30,100 30,0 70,22 30,45" },
        { "hagalaz",  "25,0 25,100|75,0 75,100|25,35 75,65" },
        { "naudiz",   "50,0 50,100|25,35 75,60" },
        { "isaz",     "50,0 50,100" },
        { "jera",     "45,15 20,40 45,65|55,35 80,60 55,85" },
        { "eihwaz",   "50,0 50,100|50,0 75,20|50,100 25,80" },
        { "perthro",  "30,0 30,100|30,0 70,20 70,35|30,100 70,80 70,65" },
        { "algiz",    "50,100 50,0|50,45 15,5|50,45 85,5" },
        { "sowilo",   "70,0 30,40 70,60 30,100" },
        { "tiwaz",    "50,100 50,0|50,0 20,30|50,0 80,30" },
        { "berkano",  "30,0 30,100|30,0 70,25 30,50 70,75 30,100" },
        { "ehwaz",    "25,100 25,0 50,30 75,0 75,100" },
        { "mannaz",   "25,100 25,0 75,45|75,100 75,0 25,45" },
        { "laguz",    "35,100 35,0 70,30" },
        { "ingwaz",   "50,10 80,50 50,90 20,50 50,10" },
        { "dagaz",    "20,0 20,100 80,0 80,100 20,0" },
        { "othala",   "20,100 80,40 50,5 20,40 80,100" },
    };

    constexpr int runeCount = (int) (sizeof (runeSpecs) / sizeof (runeSpecs[0]));

    juce::Path parseUnitPath (const char* spec)
    {
        juce::Path p;

        for (const auto& stroke : juce::StringArray::fromTokens (spec, "|", ""))
        {
            bool first = true;

            for (const auto& pt : juce::StringArray::fromTokens (stroke, " ", ""))
            {
                if (pt.isEmpty())
                    continue;

                const float x = pt.upToFirstOccurrenceOf (",", false, false).getFloatValue() / 100.0f;
                const float y = pt.fromFirstOccurrenceOf (",", false, false).getFloatValue() / 100.0f;

                if (first) { p.startNewSubPath (x, y); first = false; }
                else       { p.lineTo (x, y); }
            }
        }

        return p;
    }

    const std::vector<juce::Path>& unitRunes()
    {
        static const std::vector<juce::Path> paths = []
        {
            std::vector<juce::Path> v;
            for (const auto& spec : runeSpecs)
                v.push_back (parseUnitPath (spec.strokes));
            return v;
        }();
        return paths;
    }
}

int numRunes() noexcept { return runeCount; }

int findRune (const char* name) noexcept
{
    for (int i = 0; i < runeCount; ++i)
        if (std::strcmp (runeSpecs[i].name, name) == 0)
            return i;
    return 0;
}

const char* runeName (int index) noexcept
{
    return runeSpecs[juce::jlimit (0, runeCount - 1, index)].name;
}

juce::Path runePath (int index, juce::Rectangle<float> box)
{
    auto p = unitRunes()[(size_t) juce::jlimit (0, runeCount - 1, index)];
    p.applyTransform (juce::AffineTransform::scale (box.getWidth(), box.getHeight()).translated (box.getX(), box.getY()));
    return p;
}

void drawRune (juce::Graphics& g, int index, juce::Rectangle<float> box, juce::Colour c, float intensity, float thickness)
{
    drawGlowPath (g, runePath (index, box), c, intensity, thickness);
}
}

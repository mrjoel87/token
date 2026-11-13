#include "Theme.h"

namespace token::look
{
namespace
{
    using C = juce::Colour;

    Look makeDark()
    {
        Look l;
        l.light         = false;
        l.chassisTop    = C (0xff0d0e13);
        l.chassisBottom = C (0xff07080b);
        l.screenTop     = C (0xff0a0c14);
        l.screenBottom  = C (0xff040508);
        l.grid          = C (0x0dffffff);
        l.frame         = C (0xff1d212c);
        l.separator     = C (0xff1a1d26);
        l.text          = C (0xffeef1f8);
        l.textDim       = C (0xff8a91a3);
        l.caption       = C (0xff6c7385);
        l.pillOff       = C (0xff11141b);
        l.pillOffText   = C (0xff9aa1b2);
        l.pillEdge      = C (0xff222633);
        l.pillOn        = C (0xff262b3b);
        l.pillOnText    = C (0xffffffff);
        l.track         = C (0xff252a37);
        return l;
    }

    Look makeLight()
    {
        Look l;
        l.light         = true;
        l.chassisTop    = C (0xfff4f1ea);
        l.chassisBottom = C (0xffebe7de);
        l.screenTop     = C (0xfffbf9f4);
        l.screenBottom  = C (0xfff2eee5);
        l.grid          = C (0x141a1c22);
        l.frame         = C (0xffd5d0c5);
        l.separator     = C (0xffd9d4c9);
        l.text          = C (0xff15171c);
        l.textDim       = C (0xff4a4e57);   // ~7:1 on the chassis
        l.caption       = C (0xff5b5f68);
        l.pillOff       = C (0xffe4dfd5);
        l.pillOffText   = C (0xff3a3d45);
        l.pillEdge      = C (0xffc9c3b7);
        l.pillOn        = C (0xff15171c);   // dark selected state: unmistakable on white
        l.pillOnText    = C (0xfffbf9f4);
        l.track         = C (0xffcfc9bd);
        return l;
    }

    const Look* current = nullptr;
    juce::Colour currentAccent { 0xff7fe9ff };
}

const Look& dark()  { static const Look l = makeDark();  return l; }
const Look& light() { static const Look l = makeLight(); return l; }

void setActive (bool lightTheme) noexcept   { current = lightTheme ? &light() : &dark(); }
const Look& active() noexcept               { return current != nullptr ? *current : dark(); }
void setAccent (juce::Colour c) noexcept    { currentAccent = c; }
juce::Colour accent() noexcept              { return currentAccent; }

const Palette& studioPalette (int index)
{
    static const std::array<Palette, numStudioPalettes> set {{
        { C (0xff9ff2ff), C (0xff2f5bff), C (0xffffb35c) },   // ABYSS: cyan, electric blue, amber
        { C (0xffd4c4ff), C (0xff7a3cff), C (0xff46eeff) },   // ULTRAVIOLET: lilac, ultraviolet, cyan
        { C (0xffd8ff5a), C (0xff19e3a2), C (0xffff47e3) },   // ACID: lime, toxic green, magenta
        { C (0xffff6b6b), C (0xffb3122e), C (0xffffd38a) },   // BLOOD: red light, deep crimson, pale gold
        { C (0xffffe2a8), C (0xffff9a2e), C (0xff9ff2ff) },   // GOLD: warm light, amber, ice
    }};
    return set[(size_t) juce::jlimit (0, numStudioPalettes - 1, index)];
}

Palette manualPalette (bool lightTheme)
{
    if (lightTheme)
        return { C (0xff15171c), C (0xff2b59c3), C (0xffc9761e) };     // ink, deep blue, amber

    return { C (0xff8eeeff), C (0xff7b5cff), C (0xffffb35c) };         // cyan, violet, amber
}

const std::array<Palette, 6>& beatPalettes (bool lightTheme)
{
    static const std::array<Palette, 6> darkSet {{
        { C (0xff8eeeff), C (0xff7b5cff), C (0xffffb35c) },   // cyan / violet / amber
        { C (0xffc3b1ff), C (0xff6a3df0), C (0xffffc47a) },   // lilac / violet / gold
        { C (0xff9fdcff), C (0xff3557e0), C (0xffeef2ff) },   // ice / midnight blue / white
        { C (0xffffd59a), C (0xffff6f91), C (0xff8b6cff) },   // amber / rose / violet
        { C (0xff8ff7de), C (0xff1fa3b8), C (0xfff4f7ff) },   // mint / teal / white
        { C (0xffffa0e0), C (0xff5a5cff), C (0xff8eeeff) },   // orchid / blue / cyan
    }};

    static const std::array<Palette, 6> lightSet {{
        { C (0xff15171c), C (0xff2b59c3), C (0xffc9761e) },
        { C (0xff1d1530), C (0xff6a3df0), C (0xffcf8a2c) },
        { C (0xff0e2a3a), C (0xff0f7c94), C (0xffc74f6a) },
        { C (0xff2a1a12), C (0xffc4691a), C (0xff6a3df0) },
        { C (0xff101820), C (0xff157a6e), C (0xff2b59c3) },
        { C (0xff241020), C (0xffb8327a), C (0xff2b59c3) },
    }};

    return lightTheme ? lightSet : darkSet;
}

Palette mix (const Palette& from, const Palette& to, float t)
{
    return { from.a.interpolatedWith (to.a, t), from.b.interpolatedWith (to.b, t), from.c.interpolatedWith (to.c, t) };
}
}

#pragma once

#include <juce_graphics/juce_graphics.h>
#include <array>

/*
    TOKEN's palette. The colours are variables (not constants) so the whole instrument - chassis,
    scope, world and transmissions - can be re-themed: DARK, LIGHT, TEAL or the original GOLD.
    Only ever modified on the message thread (pal::applyTheme from the editor).
*/
namespace token::pal
{
    // Screen / world colours (named after their role in the original GOLD theme)
    inline juce::Colour black      { 0xff030407 };   // screen background
    inline juce::Colour midnight   { 0xff0a1430 };
    inline juce::Colour cyan       { 0xff46eeff };   // primary light
    inline juce::Colour turquoise  { 0xff5fd8c4 };
    inline juce::Colour gold       { 0xffc9a45c };   // engraving / secondary light
    inline juce::Colour goldBright { 0xfff2d79a };
    inline juce::Colour goldDark   { 0xff6a5226 };
    inline juce::Colour amber      { 0xffffb35a };
    inline juce::Colour violet     { 0xff6a48b8 };
    inline juce::Colour pink       { 0xffffb6cf };
    inline juce::Colour crimson    { 0xffc8283c };
    inline juce::Colour moon       { 0xffe8f0ff };
    inline juce::Colour core       { 0xffffffff };   // what a glowing line's core blends toward

    // Hardware
    inline juce::Colour chassisTop    { 0xff30343b };
    inline juce::Colour chassisSheen  { 0xff3d424a };
    inline juce::Colour chassisMid    { 0xff1e2126 };
    inline juce::Colour chassisBottom { 0xff111316 };
    inline juce::Colour panelTop      { 0xff15171b };
    inline juce::Colour panelBottom   { 0xff1d2025 };
    inline juce::Colour bezelTop      { 0xff08090b };
    inline juce::Colour bezelBottom   { 0xff23262c };

    // Sky handling
    inline juce::Colour skyLightTop     { 0xffd8d8d4 };
    inline juce::Colour skyLightHorizon { 0xffc4c5c2 };
    inline float saturation = 1.0f;     // applied to the themed skies
    inline float skyTint = 0.0f;        // pull skies toward 'midnight'
    inline float vignette = 0.6f;
    inline bool  lightScreen = false;   // LIGHT theme: dark ink on a pale screen
    inline int   currentTheme = -1;

    /** 0 = Dark, 1 = Light, 2 = Teal, 3 = Gold (original) */
    inline void applyTheme (int theme)
    {
        if (theme == currentTheme)
            return;

        currentTheme = theme;
        using C = juce::Colour;

        switch (theme)
        {
            case 1: // LIGHT - sophisticated pale monochrome
                black = C (0xffcfd0cc);  midnight = C (0xffb9bbb8);
                cyan = C (0xff1e2329);   turquoise = C (0xff3a4048);
                gold = C (0xff4a4d52);   goldBright = C (0xff15171a);  goldDark = C (0xff8c8f93);
                amber = C (0xff5a5650);  violet = C (0xff6c6f78);      pink = C (0xff6a6468);
                crimson = C (0xff3b2a2c); moon = C (0xff24272c);       core = C (0xff000000);
                chassisTop = C (0xffe9e9e6); chassisSheen = C (0xfff6f6f3); chassisMid = C (0xffd9d9d5); chassisBottom = C (0xffc3c3bf);
                panelTop = C (0xffd2d2ce);   panelBottom = C (0xffdcdcd8);
                bezelTop = C (0xffa9aaa6);   bezelBottom = C (0xffe2e2df);
                saturation = 0.0f; skyTint = 0.0f; vignette = 0.22f; lightScreen = true;
                break;

            case 2: // TEAL - deep oceanic teal and cyan
                black = C (0xff010a0c);  midnight = C (0xff03242b);
                cyan = C (0xff3ff2e6);   turquoise = C (0xff22b5a6);
                gold = C (0xff6fcfc0);   goldBright = C (0xffc6fff5);  goldDark = C (0xff1b4f4c);
                amber = C (0xff9ef0d4);  violet = C (0xff1f5c74);      pink = C (0xff9ce6dd);
                crimson = C (0xff2a9d8f); moon = C (0xffdcfbf6);       core = C (0xffffffff);
                chassisTop = C (0xff16302f); chassisSheen = C (0xff1f3f3d); chassisMid = C (0xff0f2221); chassisBottom = C (0xff071413);
                panelTop = C (0xff0b1b1b);   panelBottom = C (0xff112625);
                bezelTop = C (0xff031010);   bezelBottom = C (0xff14302e);
                saturation = 0.7f; skyTint = 0.55f; vignette = 0.6f; lightScreen = false;
                break;

            case 3: // GOLD - the original TOKEN look
                black = C (0xff030407);  midnight = C (0xff0a1430);
                cyan = C (0xff46eeff);   turquoise = C (0xff5fd8c4);
                gold = C (0xffc9a45c);   goldBright = C (0xfff2d79a);  goldDark = C (0xff6a5226);
                amber = C (0xffffb35a);  violet = C (0xff6a48b8);      pink = C (0xffffb6cf);
                crimson = C (0xffc8283c); moon = C (0xffe8f0ff);       core = C (0xffffffff);
                chassisTop = C (0xff30343b); chassisSheen = C (0xff3d424a); chassisMid = C (0xff1e2126); chassisBottom = C (0xff111316);
                panelTop = C (0xff15171b);   panelBottom = C (0xff1d2025);
                bezelTop = C (0xff08090b);   bezelBottom = C (0xff23262c);
                saturation = 1.0f; skyTint = 0.0f; vignette = 0.6f; lightScreen = false;
                break;

            case 0: // DARK - near-black with restrained silver-blue light
            default:
                black = C (0xff030405);  midnight = C (0xff0c121c);
                cyan = C (0xffa9c8ea);   turquoise = C (0xff7f9fbf);
                gold = C (0xff9aa4b2);   goldBright = C (0xffe4ebf4);  goldDark = C (0xff3e4652);
                amber = C (0xffc9d4e2);  violet = C (0xff4a5670);      pink = C (0xffc0c8d8);
                crimson = C (0xff6d7d98); moon = C (0xffe6edf7);       core = C (0xffffffff);
                chassisTop = C (0xff1b1d21); chassisSheen = C (0xff26292e); chassisMid = C (0xff111215); chassisBottom = C (0xff08090a);
                panelTop = C (0xff0c0d0f);   panelBottom = C (0xff131518);
                bezelTop = C (0xff040405);   bezelBottom = C (0xff17191c);
                saturation = 0.25f; skyTint = 0.35f; vignette = 0.65f; lightScreen = false;
                break;
        }
    }

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

    /** Applies the current theme to a fixed 'world' colour (used for skies and landscapes). */
    inline juce::Colour themed (juce::Colour c)
    {
        if (lightScreen)
            return c.withSaturation (0.0f).withBrightness (1.0f - c.getBrightness() * 0.25f - 0.1f);

        return c.withMultipliedSaturation (saturation).interpolatedWith (midnight, skyTint);
    }
}

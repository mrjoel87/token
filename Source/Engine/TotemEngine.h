#pragma once

#include "SignalFrame.h"
#include "Sketch.h"
#include "ScopePen.h"
#include "Glyphs.h"
#include "Transmissions.h"
#include "../ColourEngine.h"
#include "../ScopeBuffers.h"
#include <vector>

namespace token::engine
{
/** The settings the editor hands over each frame (read from the parameters). */
struct EngineSettings
{
    float speed = 1.0f;       // already mapped through speedFactor()
    float react = 0.65f;      // 0..1
    bool locked = false;
    bool strip = false;
    bool light = false;
    int quality = 2;
};

/**
    TOTEM 2: one living oscilloscope.

    Everything on screen is the signal: a spine trace, breathing rings, a summoned image at the
    centre (one of the glyph library, drawn on line by line), smaller images that pop up around it,
    and messages from nowhere. Kicks pulse it, volume makes it breathe, snares split it into
    colours, drops shatter it into copies, and every few bars it flips through itself.
    All buffers are allocated once; nothing is allocated per frame.
*/
class TotemEngine
{
public:
    TotemEngine();

    void update (const ScopeHistory&, const AudioFeatures&, const ColourEngine&, const EngineSettings&,
                 juce::Rectangle<float> area, double sampleRate, float dt, float time);
    void draw (juce::Graphics&) const;

    const SignalFrame& frame() const noexcept { return sf; }
    const char* currentName() const;
    int currentNumber() const noexcept { return hero.glyph + 1; }
    int librarySize() const;

private:
    struct Summon
    {
        int glyph = -1;
        float age = 0.0f, life = 12.0f;
        juce::uint32 seed = 1;
        int slot = 0;
    };

    void extractTriggered (const ScopeHistory&, float gain, float peak, double sampleRate, int displayLen);
    void deriveShapes (float dt);
    int nextFromBag();
    void summonHero();
    GlyphCtx contextFor (const Summon&) const;
    void drawSummon (juce::Graphics&, const Summon&, juce::Point<float> centre, float radius, bool isHero) const;

    SignalFrame sf;
    Transmissions transmissions;

    Summon hero, leaving;
    std::array<Summon, 2> satellites;
    float leavingAge = -1.0f, satelliteGap = 5.0f;
    std::vector<int> bag;
    juce::Random rng;

    std::array<float, SignalFrame::N> scratch {};
    std::vector<float> lpScratch;
    std::vector<int> negScratch;
    float evo = 0.0f, motion = 0.0f, spin = 0.0f, lastDrop = 0.0f;
    float flip = 0.0f, flipTarget = 0.0f, flipClock = 0.0f;
    float glitch = 0.0f, shatter = 0.0f;
    long long lastFlipBar = -1;
    bool first = true;

    mutable Sketch sketch;
    mutable ScopePen pen;
    mutable juce::Path heroPath, workPath, spinePath;
};
}

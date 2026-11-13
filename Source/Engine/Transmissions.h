#pragma once

#include "ScopePen.h"

namespace token::engine
{
/**
    Messages from nowhere. Every so often a short line arrives: it decodes from scrambled
    characters, holds, then breaks apart. The letters are drawn with the scope pen, so the text is
    made of the signal too. A faint "numbers station" readout flickers in a corner on beats.
*/
class Transmissions
{
public:
    Transmissions();

    void update (const SignalFrame&);
    void draw (juce::Graphics&, const SignalFrame&, juce::Colour ink, juce::Colour glow, juce::Colour dim) const;

    bool active() const noexcept { return age >= 0.0f; }

private:
    void begin (const SignalFrame&);
    void rebuildText (const SignalFrame&, const juce::String&) const;

    juce::Random rng;
    juce::String message;
    float age = -1.0f, life = 6.0f, gap = 25.0f;
    int lastPhrase = -1;
    float stationClock = 0.0f;
    juce::String station;

    mutable Sketch letters;
    mutable ScopePen pen;
    mutable juce::Path path;
    mutable juce::String builtFor;
    mutable juce::Rectangle<float> builtArea;
};
}

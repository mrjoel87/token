#pragma once

#include "Sketch.h"
#include <vector>

namespace token::engine
{
/** What a glyph may animate with (all already smoothed, 0..1 unless noted). */
struct GlyphCtx
{
    float t = 0.0f;           // seconds on screen (SPEED-scaled; frozen by LOCK)
    float spin = 0.0f;        // rotation for 3D / 4D objects (radians, frozen by LOCK)
    float beatPhase = 0.0f;
    float kick = 0.0f, bass = 0.0f, high = 0.0f, energy = 0.0f, volume = 0.0f;
    juce::uint32 seed = 1;
};

enum class Realm { Occult, Sacred, Dimension, Strange };

using GlyphFn = void (*) (Sketch&, const GlyphCtx&);

struct Glyph
{
    const char* name;
    GlyphFn draw;
    Realm realm;
    float cycles;             // how many waveform repeats ride along the whole drawing
};

/** The 40 images TOTEM 2 can summon. All drawn in unit space (about -1..1). */
const std::vector<Glyph>& glyphLibrary();
}

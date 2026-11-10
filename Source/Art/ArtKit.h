#pragma once

#include <juce_graphics/juce_graphics.h>
#include <vector>

/*
    SMSW TOTEM - artwork library.

    Every element is a small, original drawing made from fine lines, monochrome silhouettes, soft
    glows and restrained pixel art, coloured by the current theme palette. Elements are drawn only
    while they are on screen (no image files, nothing cached per element).

    To add an element: write an ArtFn in one of the Art*.cpp files and add one Spec line to that
    file's add...() function.
*/
namespace token::art
{
enum class Category { Ancient = 0, Nature, Celestial, Sentimental, Surreal, Geometric };
constexpr int numCategories = 6;

/** Where an element may be placed. Band* zones span the full width (for things that travel across). */
enum class Zone { Sky, Ground, Centre, Anywhere, Full, BandSky, BandMid, BandLow };

enum Flags
{
    flagRare      = 1,    // appears far less often
    flagNoFlip    = 2,    // never mirrored (symbols, text-like glyphs)
    flagHero      = 4,    // may be the main element
    flagCompanion = 8,    // may accompany the main element (small, ambient)
    flagSpin      = 16    // radially symmetric: the engine turns it in steps locked to the beat
};

/** Colours an element is painted with (derived from theme + palette). */
struct Ink
{
    juce::Colour line;     // fine lines and outlines
    juce::Colour glow;     // halos around lines
    juce::Colour accent;   // small highlights
    juce::Colour sil;      // silhouettes
    juce::Colour haze;     // atmospheric light (moon glow, fog, horizon)
    juce::Colour warm;     // lamps, windows
    juce::Colour bg;       // the screen colour behind
    bool light = false;
};

/** Everything an element needs to draw one frame. */
struct Frame
{
    juce::Rectangle<float> box;
    float t = 0.0f;           // seconds on screen
    float life = 0.0f;        // 0..1 through its lifetime
    float px = 2.0f;          // pixel-art unit
    float bass = 0.0f, mid = 0.0f, high = 0.0f, kick = 0.0f, hat = 0.0f;
    float energy = 0.0f, flow = 0.0f, drop = 0.0f, beat = 0.0f;
    float beats = 0.0f;       // continuous beat count (host song position, or the onset-aligned clock)
    float bar = 0.0f;         // 0..1 within the bar
    float travel = 0.0f;      // forward motion in beats, scaled by SPEED and lifted by the bass
    float speed = 1.0f;       // SPEED multiplier
    const float* spectrum = nullptr;   // log-spaced spectrum, 0..1 per band
    int bands = 0;
    juce::uint32 seed = 1;
    bool flip = false;
    Ink ink;

    float X (float u) const noexcept { return box.getX() + (flip ? 1.0f - u : u) * box.getWidth(); }
    float Y (float v) const noexcept { return box.getY() + v * box.getHeight(); }
    float DX (float u) const noexcept { return (flip ? -u : u) * box.getWidth(); }
    float W() const noexcept { return box.getWidth(); }
    float H() const noexcept { return box.getHeight(); }
    float S() const noexcept { return juce::jmin (box.getWidth(), box.getHeight()); }
    juce::Point<float> P (float u, float v) const noexcept { return { X (u), Y (v) }; }
};

using ArtFn = void (*) (juce::Graphics&, const Frame&);

/** How well an element suits the music: quiet / bass-heavy / melodic / energetic (0..1). */
struct Affinity
{
    float quiet, bass, melodic, energetic;
};

struct Spec
{
    const char* name;
    Category category;
    ArtFn draw;
    Zone zone;
    float size;              // height as a fraction of the visual area (bands: band height)
    float aspect;            // width / height of the element's box (ignored for bands and Full)
    float minLife, maxLife;  // seconds on screen
    Affinity affinity;
    int flags;
};

void addAncient     (std::vector<Spec>&);
void addNature      (std::vector<Spec>&);
void addCelestial   (std::vector<Spec>&);
void addSentimental (std::vector<Spec>&);
void addSurreal     (std::vector<Spec>&);
void addGeometric   (std::vector<Spec>&);
void addMandala     (std::vector<Spec>&);

const std::vector<Spec>& library();

// =================================================================================================
//  Drawing kit (always call with the kit:: prefix)
// =================================================================================================
namespace kit
{
    enum class Pose { Stand, Walk, Sit };

    float hash (juce::uint32 seed, int i) noexcept;          // 0..1, deterministic
    float noise (juce::uint32 seed, float x) noexcept;       // smooth 0..1
    float ease (float t) noexcept;                           // smoothstep, clamped
    float beatStep (float beats, float beatsPerStep) noexcept; // whole steps + an eased glide on each beat
    float spectrumAt (const Frame&, float u) noexcept;       // 0..1, u = 0 (bass) .. 1 (treble)

    juce::PathStrokeType pen (float width);
    void halo (juce::Graphics&, juce::Point<float> centre, float radius, juce::Colour centreColour);
    void blob (juce::Graphics&, juce::Point<float> centre, float rx, float ry, juce::Colour centreColour);
    void glowLine (juce::Graphics&, const juce::Path&, juce::Colour core, juce::Colour glowColour,
                   float width, float glowAmount, bool lightTheme);
    void silhouette (juce::Graphics&, const juce::Path&, const Frame&, float outlineGlow);

    juce::Path ridge (const Frame&, juce::uint32 seed, float baseV, float ampV, float jagged, int detail);
    juce::Path polygon (juce::Point<float> centre, int sides, float radius, float rotation);
    juce::Path unitPath (const char* spec, juce::Rectangle<float> box);   // "x,y x,y|..." in percent

    void fog (juce::Graphics&, const Frame&, float yV, float hV, juce::Colour);
    void pine (juce::Graphics&, float x, float groundY, float h, juce::Colour, float sway);
    void pixelRect (juce::Graphics&, float x, float y, float w, float h, juce::Colour);
    void person (juce::Graphics&, float x, float groundY, float px, juce::Colour, Pose, float phase, float facing);
    void stars (juce::Graphics&, const Frame&, juce::uint32 seed, int n, float maxV, juce::Colour, float twinkle);
    void rain (juce::Graphics&, const Frame&, juce::uint32 seed, int n, juce::Colour, float slant, float topV);
    void snow (juce::Graphics&, const Frame&, juce::uint32 seed, int n, juce::Colour);
    void cloud (juce::Graphics&, float cx, float cy, float w, float h, juce::Colour body, juce::Colour rim, juce::uint32 seed);
    void bird (juce::Graphics&, float x, float y, float size, float flap, juce::Colour);
    void lamp (juce::Graphics&, float x, float groundY, float h, juce::Colour pole, juce::Colour light, float intensity);
    void moon (juce::Graphics&, juce::Point<float> centre, float r, juce::Colour, float haloAmount);
    void reflection (juce::Graphics&, float x, float y, float width, float depth, juce::Colour, float t, juce::uint32 seed);
    void sevenSeg (juce::Graphics&, int digit, juce::Rectangle<float> box, juce::Colour, float thickness);
}
}

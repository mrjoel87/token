#pragma once

#include <juce_graphics/juce_graphics.h>
#include <vector>

/*
    The Hidden Transmission scene library.

    Every scene is a small procedural "photograph" drawn with vector primitives in the theme's
    monochrome inks. Scenes are rendered into a reduced-size off-screen image each frame and the
    TransmissionEngine composites them INTO the signal (waveform masking, interference, ghosting).

    To add a scene: write a SceneFn in one of the Scenes*.cpp files and add a SceneSpec line to that
    file's add...Scenes() function. Nothing else needs to change.
*/
namespace token::tx
{
enum class Category { Lost = 0, Occult, Human, Nature, Roads };
constexpr int numCategories = 5;

/** Monochrome inks derived from the current theme. */
struct Ink
{
    juce::Colour deep;     // the screen's darkest tone
    juce::Colour haze;     // lit atmosphere: fog, horizon glow, water light
    juce::Colour sil;      // silhouettes
    juce::Colour mid;      // distant shapes
    juce::Colour light;    // highlights, lit edges
    juce::Colour accent;   // the theme's signature glow
    juce::Colour warm;     // lamps, windows, embers
};

struct SceneParams
{
    float t = 0.0f;              // seconds since the scene appeared
    float beat = 0.0f;           // continuous beat position (host ppq, or a free-running clock)
    float bass = 0.0f, mid = 0.0f, high = 0.0f, kick = 0.0f, snare = 0.0f, intensity = 0.0f;
    float mystery = 0.5f;
    juce::uint32 seed = 1;
    int variant = 0;
    Ink ink;
};

using SceneFn = void (*) (juce::Graphics&, juce::Rectangle<float>, const SceneParams&);

/** How well a scene suits each musical profile (0..1). */
struct Affinity
{
    float ambient, rhythm, sparse, dark, intense;
};

struct SceneSpec
{
    const char* name;
    Category category;
    SceneFn draw;
    int variant;
    Affinity affinity;
    bool rare;
};

void addLostScenes   (std::vector<SceneSpec>&);
void addOccultScenes (std::vector<SceneSpec>&);
void addHumanScenes  (std::vector<SceneSpec>&);
void addNatureScenes (std::vector<SceneSpec>&);
void addRoadScenes   (std::vector<SceneSpec>&);

/** The full library (built once). */
const std::vector<SceneSpec>& sceneLibrary();

// =================================================================================================
//  Drawing kit shared by all scenes
// =================================================================================================
namespace kit
{
    float hash (juce::uint32 seed, int i) noexcept;            // deterministic 0..1
    float noise1 (juce::uint32 seed, float x) noexcept;        // smooth value noise 0..1

    struct Frame
    {
        juce::Rectangle<float> r;
        float X (float f) const noexcept { return r.getX() + f * r.getWidth(); }
        float Y (float f) const noexcept { return r.getY() + f * r.getHeight(); }
        float W() const noexcept { return r.getWidth(); }
        float H() const noexcept { return r.getHeight(); }
    };

    void sky (juce::Graphics&, const Frame&, juce::Colour top, juce::Colour horizon, juce::Colour bottom, float horizonFrac);
    void glow (juce::Graphics&, juce::Point<float> centre, float radius, juce::Colour centreColour);
    void softEllipse (juce::Graphics&, juce::Point<float> centre, float radiusX, float radiusY, juce::Colour centreColour);
    juce::Path ridge (const Frame&, juce::uint32 seed, float baseFrac, float ampFrac, float jagged, int detail);
    void fogBand (juce::Graphics&, const Frame&, float yFrac, float hFrac, juce::Colour, float t, juce::uint32 seed);
    void figure (juce::Graphics&, float x, float groundY, float h, juce::Colour, float walkPhase, bool walking);
    void pine (juce::Graphics&, float x, float groundY, float h, juce::Colour, float sway);
    void trees (juce::Graphics&, const Frame&, juce::uint32 seed, int n, float groundFrac, float minH, float maxH,
                juce::Colour, float t, float sway);
    void moon (juce::Graphics&, juce::Point<float> centre, float r, juce::Colour);
    void stars (juce::Graphics&, const Frame&, juce::uint32 seed, int n, float maxYFrac, juce::Colour, float t);
    void rain (juce::Graphics&, const Frame&, juce::uint32 seed, float t, int n, juce::Colour, float slant);
    void snow (juce::Graphics&, const Frame&, juce::uint32 seed, float t, int n, juce::Colour);
    void streetlight (juce::Graphics&, float x, float groundY, float h, juce::Colour pole, juce::Colour lamp,
                      float intensity, bool cone);
    void reflection (juce::Graphics&, float x, float y, float width, float depth, juce::Colour, float t, juce::uint32 seed);
    void glowStroke (juce::Graphics&, const juce::Path&, juce::Colour, float thickness, float glowAmount);
    void glyph (juce::Graphics&, const char* spec, juce::Rectangle<float> box, juce::Colour, float thickness, float glowAmount);
    void sevenSegment (juce::Graphics&, int digit, juce::Rectangle<float> box, juce::Colour, float thickness);
    void bird (juce::Graphics&, float x, float y, float size, float flap, juce::Colour);
    void cloud (juce::Graphics&, float cx, float cy, float w, float h, juce::Colour body, juce::Colour rim, juce::uint32 seed);
}
}

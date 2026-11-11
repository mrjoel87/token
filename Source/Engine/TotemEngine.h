#pragma once

#include "SignalFrame.h"
#include "SacredWorld.h"
#include "DreamWorld.h"
#include "SignalWorld.h"
#include "Alignment.h"
#include "../ColourEngine.h"
#include "../ScopeBuffers.h"
#include <vector>

namespace token::engine
{
/** The settings the editor hands over each frame (read from the parameters). */
struct EngineSettings
{
    World world = World::Sacred;
    float speed = 1.0f;       // already mapped through speedFactor()
    float react = 0.65f;      // 0..1
    bool locked = false;
    bool alignment = true;
    bool strip = false;
    int quality = 2;
};

/**
    Owns the three worlds and the alignment director, and turns the incoming audio into one
    SignalFrame per display frame. All buffers are allocated once; nothing is allocated per frame.
*/
class TotemEngine
{
public:
    TotemEngine();

    void update (const ScopeHistory&, const AudioFeatures&, const ColourEngine&, const EngineSettings&,
                 juce::Rectangle<float> area, double sampleRate, float dt, float time);
    void draw (juce::Graphics&) const;

    const SignalFrame& frame() const noexcept { return sf; }
    float alignmentLevel() const noexcept { return alignment.amount(); }

private:
    void extractTriggered (const ScopeHistory&, float gain, float peak, double sampleRate, int displayLen);
    void deriveShapes (float dt);

    SignalFrame sf;
    SacredWorld sacred;
    DreamWorld dream;
    SignalWorld signalWorld;
    AlignmentDirector alignment;

    std::array<float, 3> weight { 1.0f, 0.0f, 0.0f };
    std::array<float, SignalFrame::N> scratch {};
    std::vector<float> lpScratch;
    std::vector<int> negScratch;
    float evo = 0.0f, motion = 0.0f, lastDrop = 0.0f;
    bool first = true;
};
}

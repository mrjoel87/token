#pragma once

#include "WorldTypes.h"
#include "../Parameters.h"
#include <array>
#include <vector>

namespace token
{
/**
    TOKEN's living world: themed backgrounds (Dream / Mystic / Norse), parallax landscapes, stars,
    aurora, ambient particles, kick shockwaves and the weighted-random event scheduler.
*/
class WorldEngine
{
public:
    WorldEngine();

    void update (WorldContext&, SceneMode);

    void drawBackground (juce::Graphics&, const WorldContext&) const;
    void drawMidEffects (juce::Graphics&, const WorldContext&) const;
    void drawEvents (juce::Graphics&, const WorldContext&, Layer) const;
    void drawParticles (juce::Graphics&) const;

    const std::array<float, 3>& getTheme() const noexcept { return theme; }
    ParticleSystem& getParticles() noexcept { return particles; }

private:
    struct Star  { float x, y, size, phase, depth, flashy; };
    struct Shock { float age = 10.0f, strength = 0.0f; };

    void spawnAmbient (const WorldContext&);
    void schedule (const WorldContext&);
    int pickWeighted (const WorldContext&);
    bool trySpawn (const char* name, const WorldContext&);
    void spawn (int index, const WorldContext&);
    int maxEvents (const WorldContext&) const;

    void drawSkyline (juce::Graphics&, const WorldContext&, int themeIndex, float weight) const;
    void drawAurora (juce::Graphics&, const WorldContext&, float amount) const;
    void drawMoon (juce::Graphics&, const WorldContext&, juce::Colour skyTop) const;
    void drawGraticule (juce::Graphics&, const WorldContext&) const;

    std::vector<EventDef> library;
    std::vector<std::unique_ptr<WorldEvent>> active;
    std::vector<float> weightScratch;
    ParticleSystem particles;
    juce::Random rng;

    std::array<float, 3> theme { 1.0f, 0.0f, 0.0f };
    int autoTheme = 0;
    float autoThemeTimer = 75.0f, rollTimer = 2.0f, lastSpawnTime = -100.0f;
    float scroll = 0.0f, auroraPhase = 0.0f;
    float sparkleAcc = 0.0f, dustAcc = 0.0f, snowAcc = 0.0f, emberAcc = 0.0f, sandAcc = 0.0f;
    bool minimal = false;

    std::vector<Star> stars;
    std::array<std::vector<float>, 3> skylines;
    std::array<Shock, 8> shocks {};
    int shockHead = 0;
};
}

#pragma once

#include <juce_graphics/juce_graphics.h>
#include <vector>

namespace token
{
enum class ParticleType : juce::uint8 { Spark, Rain, Snow, Dust, Ember, Pixel };

struct Particle
{
    juce::Point<float> pos, vel;
    float life = 0.0f, maxLife = 1.0f, size = 1.0f, seed = 0.0f;
    juce::Colour colour;
    ParticleType type = ParticleType::Spark;
    bool alive = false;
};

/** Fixed-size pool: no allocation after construction; oldest particles are recycled when full. */
class ParticleSystem
{
public:
    ParticleSystem();

    void spawn (ParticleType, juce::Point<float> pos, juce::Point<float> vel, float life, float size, juce::Colour);
    void update (float dt, juce::Rectangle<float> bounds, float wind);
    void draw (juce::Graphics&) const;
    int getNumAlive() const noexcept { return numAlive; }

private:
    static constexpr int capacity = 1200;
    std::vector<Particle> pool;
    int cursor = 0, numAlive = 0;
    float clock = 0.0f;
};
}

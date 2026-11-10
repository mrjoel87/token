#include "Particles.h"
#include <cmath>

namespace token
{
ParticleSystem::ParticleSystem() : pool ((size_t) capacity) {}

void ParticleSystem::spawn (ParticleType type, juce::Point<float> pos, juce::Point<float> vel,
                            float life, float size, juce::Colour colour)
{
    Particle* slot = nullptr;

    for (int i = 0; i < 64 && slot == nullptr; ++i)
    {
        auto& p = pool[(size_t) cursor];
        cursor = (cursor + 1) % capacity;
        if (! p.alive)
            slot = &p;
    }

    if (slot == nullptr)
    {
        slot = &pool[(size_t) cursor];
        cursor = (cursor + 1) % capacity;
    }

    slot->pos = pos;
    slot->vel = vel;
    slot->life = 0.0f;
    slot->maxLife = juce::jmax (0.05f, life);
    slot->size = size;
    slot->seed = std::fmod ((float) cursor * 0.61803f, 1.0f);
    slot->colour = colour;
    slot->type = type;
    slot->alive = true;
}

void ParticleSystem::update (float dt, juce::Rectangle<float> b, float wind)
{
    clock += dt;
    numAlive = 0;
    const auto outer = b.expanded (60.0f);
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    for (auto& p : pool)
    {
        if (! p.alive)
            continue;

        p.life += dt;

        switch (p.type)
        {
            case ParticleType::Rain:
                p.vel.x += (wind - p.vel.x) * 0.5f * dt;
                break;
            case ParticleType::Snow:
                p.pos.x += (wind * 0.5f + std::sin (clock * 1.3f + p.seed * twoPi) * 8.0f) * dt;
                break;
            case ParticleType::Dust:
                p.vel *= std::exp (-dt * 0.3f);
                p.pos.y += std::sin (clock * 0.7f + p.seed * twoPi) * 3.0f * dt;
                break;
            case ParticleType::Ember:
                p.vel.x += std::sin (clock * 2.0f + p.seed * twoPi) * 20.0f * dt;
                p.vel.y -= 4.0f * dt;
                break;
            case ParticleType::Spark:
                p.vel *= std::exp (-dt * 2.2f);
                break;
            case ParticleType::Pixel:
                p.vel *= std::exp (-dt * 1.5f);
                break;
        }

        p.pos += p.vel * dt;

        if (p.life >= p.maxLife || ! outer.contains (p.pos)
            || (p.type == ParticleType::Rain && p.pos.y > b.getBottom()))
            p.alive = false;
        else
            ++numAlive;
    }
}

void ParticleSystem::draw (juce::Graphics& g) const
{
    constexpr float pi = juce::MathConstants<float>::pi;

    for (const auto& p : pool)
    {
        if (! p.alive)
            continue;

        const float t = juce::jlimit (0.0f, 1.0f, p.life / p.maxLife);
        const float s = p.size;

        switch (p.type)
        {
            case ParticleType::Spark:
            {
                const float a = std::pow (1.0f - t, 1.5f);
                g.setColour (p.colour.withMultipliedAlpha (a * 0.22f));
                g.fillEllipse (p.pos.x - s * 2.0f, p.pos.y - s * 2.0f, s * 4.0f, s * 4.0f);
                g.setColour (p.colour.withMultipliedAlpha (a));
                g.fillEllipse (p.pos.x - s * 0.5f, p.pos.y - s * 0.5f, s, s);
                break;
            }
            case ParticleType::Rain:
            {
                const float a = juce::jmin (1.0f, (1.0f - t) * 3.0f);
                g.setColour (p.colour.withMultipliedAlpha (a));
                g.drawLine (p.pos.x, p.pos.y, p.pos.x - p.vel.x * 0.02f, p.pos.y - p.vel.y * 0.02f, s);
                break;
            }
            case ParticleType::Snow:
            case ParticleType::Pixel:
            {
                const float a = p.type == ParticleType::Snow ? std::sin (pi * t) : (1.0f - t);
                g.setColour (p.colour.withMultipliedAlpha (a));
                g.fillRect (p.pos.x, p.pos.y, s, s);
                break;
            }
            case ParticleType::Dust:
            {
                const float a = std::sin (pi * t);
                g.setColour (p.colour.withMultipliedAlpha (a));
                g.fillEllipse (p.pos.x - s * 0.5f, p.pos.y - s * 0.5f, s, s);
                break;
            }
            case ParticleType::Ember:
            {
                const float a = (1.0f - t) * juce::jmin (1.0f, t * 6.0f);
                g.setColour (p.colour.withMultipliedAlpha (a * 0.25f));
                g.fillEllipse (p.pos.x - s * 1.5f, p.pos.y - s * 1.5f, s * 3.0f, s * 3.0f);
                g.setColour (p.colour.withMultipliedAlpha (a));
                g.fillEllipse (p.pos.x - s * 0.5f, p.pos.y - s * 0.5f, s, s);
                break;
            }
        }
    }
}
}

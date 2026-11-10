#pragma once

#include <juce_core/juce_core.h>
#include <cmath>

namespace token
{
    /** Frame-rate independent one-pole smoothing with separate rise / fall time constants (seconds). */
    inline float smoothTo (float current, float target, float dt, float attack, float release) noexcept
    {
        const float tau = target > current ? attack : release;
        return current + (target - current) * (1.0f - std::exp (-dt / juce::jmax (1.0e-4f, tau)));
    }

    inline float smoothTo (float current, float target, float dt, float tau) noexcept
    {
        return smoothTo (current, target, dt, tau, tau);
    }

    inline float smoothstep (float t) noexcept
    {
        t = juce::jlimit (0.0f, 1.0f, t);
        return t * t * (3.0f - 2.0f * t);
    }

    inline float positiveMod (float x, float m) noexcept
    {
        const float r = std::fmod (x, m);
        return r < 0.0f ? r + m : r;
    }
}

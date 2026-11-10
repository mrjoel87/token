#pragma once

#include <juce_core/juce_core.h>
#include <algorithm>
#include <vector>

namespace token
{
/**
    Lock-free single-producer / single-consumer stereo sample FIFO.
    The audio thread pushes (never blocks, never allocates); the UI thread pulls.
    If the UI is not draining (editor closed) new samples are simply dropped.
*/
class ScopeFifo
{
public:
    static constexpr int capacity = 1 << 15;

    ScopeFifo() : left ((size_t) capacity, 0.0f), right ((size_t) capacity, 0.0f) {}

    void push (const float* l, const float* r, int num) noexcept
    {
        int s1, b1, s2, b2;
        fifo.prepareToWrite (num, s1, b1, s2, b2);

        if (b1 > 0)
        {
            std::copy (l, l + b1, left.data() + s1);
            std::copy (r, r + b1, right.data() + s1);
        }
        if (b2 > 0)
        {
            std::copy (l + b1, l + b1 + b2, left.data() + s2);
            std::copy (r + b1, r + b1 + b2, right.data() + s2);
        }

        fifo.finishedWrite (b1 + b2);
    }

    int pull (float* outL, float* outR, int maxNum) noexcept
    {
        int s1, b1, s2, b2;
        fifo.prepareToRead (juce::jmin (maxNum, fifo.getNumReady()), s1, b1, s2, b2);

        if (b1 > 0)
        {
            std::copy (left.data() + s1,  left.data() + s1 + b1,  outL);
            std::copy (right.data() + s1, right.data() + s1 + b1, outR);
        }
        if (b2 > 0)
        {
            std::copy (left.data() + s2,  left.data() + s2 + b2,  outL + b1);
            std::copy (right.data() + s2, right.data() + s2 + b2, outR + b1);
        }

        fifo.finishedRead (b1 + b2);
        return b1 + b2;
    }

private:
    std::vector<float> left, right;
    juce::AbstractFifo fifo { capacity };
};

/**
    UI-side history of the most recent samples (owned and used only by the message thread).
    Samples are addressed by absolute index: valid range is [oldest(), getTotal()).
*/
class ScopeHistory
{
public:
    static constexpr int size = 1 << 16;
    static constexpr juce::int64 mask = size - 1;

    ScopeHistory() : l ((size_t) size, 0.0f), r ((size_t) size, 0.0f) {}

    void append (const float* a, const float* b, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const auto idx = (size_t) ((total + i) & mask);
            l[idx] = a[i];
            r[idx] = b[i];
        }
        total += n;
    }

    juce::int64 getTotal() const noexcept { return total; }
    juce::int64 oldest() const noexcept   { return juce::jmax ((juce::int64) 0, total - (juce::int64) size + 1); }

    float left  (juce::int64 i) const noexcept { return i < 0 ? 0.0f : l[(size_t) (i & mask)]; }
    float right (juce::int64 i) const noexcept { return i < 0 ? 0.0f : r[(size_t) (i & mask)]; }
    float mono  (juce::int64 i) const noexcept { return 0.5f * (left (i) + right (i)); }

private:
    std::vector<float> l, r;
    juce::int64 total = 0;
};
}

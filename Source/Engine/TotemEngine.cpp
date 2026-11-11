#include "TotemEngine.h"
#include "../Util.h"

namespace token::engine
{
TotemEngine::TotemEngine()
{
    // the trigger search never looks back further than this, so these never grow later
    lpScratch.resize ((size_t) ScopeHistory::size);
    negScratch.resize ((size_t) ScopeHistory::size);
}

// -------------------------------------------------------------------------------------------------
//  Signal extraction: a zero-crossing-triggered window, so periodic sound stands still
// -------------------------------------------------------------------------------------------------
void TotemEngine::extractTriggered (const ScopeHistory& h, float gain, float peak, double sr, int displayLen)
{
    const int n = SignalFrame::N;
    const auto total = h.getTotal();
    const auto oldest = h.oldest();
    const juce::int64 latestStart = total - displayLen - 1;
    const juce::int64 searchFrom = juce::jmax (oldest + 2, latestStart - displayLen);
    double startPos = (double) latestStart;

    if (latestStart > searchFrom + 8)
    {
        const int len = (int) juce::jmin ((juce::int64) lpScratch.size(), latestStart - searchFrom + 1);
        const float coeff = 1.0f - std::exp (-fx::twoPi * 250.0f / (float) sr);
        const float threshold = 0.08f * juce::jmax (1.0e-4f, peak);
        float lp = h.mono (searchFrom - 1);
        int lastNeg = -1;

        for (int i = 0; i < len; ++i)
        {
            lp += (h.mono (searchFrom + i) - lp) * coeff;
            lpScratch[(size_t) i] = lp;
            if (lp < -threshold)
                lastNeg = i;
            negScratch[(size_t) i] = lastNeg;
        }

        for (int i = len - 1; i >= 1; --i)
        {
            const float p0 = lpScratch[(size_t) (i - 1)], p1 = lpScratch[(size_t) i];
            const int neg = negScratch[(size_t) (i - 1)];
            if (p0 < 0.0f && p1 >= 0.0f && neg >= 0 && (i - neg) < displayLen / 2)
            {
                startPos = (double) (searchFrom + i - 1) + (double) (p0 / (p0 - p1));
                break;
            }
        }
    }

    const double step = (double) displayLen / (double) (n - 1);
    for (int c = 0; c < n; ++c)
    {
        const double pos = startPos + (double) c * step;
        const auto i0 = (juce::int64) std::floor (pos);
        const float fr = (float) (pos - (double) i0);
        const float v0 = (i0 < oldest || i0 >= total) ? 0.0f : h.mono (i0);
        const float v1 = (i0 + 1 < oldest || i0 + 1 >= total) ? 0.0f : h.mono (i0 + 1);
        sf.wave[(size_t) c] = std::tanh ((v0 + (v1 - v0) * fr) * gain);
    }
}

namespace
{
    using Shape = std::array<float, SignalFrame::N>;

    /** Moving average (radius 8) with clamped edges. */
    void boxBlur (const Shape& src, Shape& dst)
    {
        const int last = SignalFrame::N - 1;
        const int radius = 8;
        float acc = 0.0f;
        for (int i = -radius; i <= radius; ++i)
            acc += src[(size_t) juce::jlimit (0, last, i)];
        for (int i = 0; i <= last; ++i)
        {
            dst[(size_t) i] = acc / (float) (radius * 2 + 1);
            acc += src[(size_t) juce::jmin (last, i + radius + 1)] - src[(size_t) juce::jmax (0, i - radius)];
        }
    }
}

void TotemEngine::deriveShapes (float dt)
{
    const int n = SignalFrame::N;

    // low: two passes of a moving average (a soft, bass-weighted shape)
    boxBlur (sf.wave, scratch);
    boxBlur (scratch, sf.low);

    for (int i = 0; i < n; ++i)
        sf.fine[(size_t) i] = sf.wave[(size_t) i] - sf.low[(size_t) i];

    // slow: flows more lazily when the sound is sustained (pads), snaps when it is percussive
    const float k = first ? 1.0f : 1.0f - std::exp (-dt / (0.05f + 0.25f * sf.sustain));
    for (int i = 0; i < n; ++i)
        sf.slow[(size_t) i] += (sf.wave[(size_t) i] - sf.slow[(size_t) i]) * k;
}

// -------------------------------------------------------------------------------------------------
void TotemEngine::update (const ScopeHistory& h, const AudioFeatures& f, const ColourEngine& colours,
                          const EngineSettings& set, juce::Rectangle<float> area, double sampleRate, float dt, float time)
{
    const double sr = juce::jmax (8000.0, sampleRate);
    const float react = juce::jlimit (0.0f, 1.0f, set.react);
    const float k = 0.3f + 1.2f * react;   // REACT scales how hard the music moves things

    // musical features, smoothed where they drive geometry
    sf.audio = &f;
    sf.bass    = smoothTo (sf.bass, juce::jlimit (0.0f, 1.0f, (0.5f * f.bass + 0.5f * f.bassRel) * k), dt, 0.04f, 0.25f);
    sf.mid     = smoothTo (sf.mid, juce::jlimit (0.0f, 1.0f, (0.5f * f.mid + 0.5f * f.midRel) * k), dt, 0.1f, 0.4f);
    sf.high    = smoothTo (sf.high, juce::jlimit (0.0f, 1.0f, f.highRel * k), dt, 0.02f, 0.2f);
    sf.kick    = juce::jlimit (0.0f, 1.0f, f.kick * k);
    sf.snare   = juce::jlimit (0.0f, 1.0f, f.snare * k);
    sf.hat     = juce::jlimit (0.0f, 1.0f, f.hat * k);
    sf.sustain = smoothTo (sf.sustain, f.sustain * (1.0f - juce::jlimit (0.0f, 1.0f, f.onsetRate / 4.0f)), dt, 0.8f, 1.2f);
    sf.quiet   = smoothTo (sf.quiet, juce::jlimit (0.0f, 1.0f, f.silence * 0.7f + f.breakdown * 0.6f + (1.0f - f.loudness) * 0.3f - 0.15f), dt, 1.2f, 0.4f);
    sf.energy  = smoothTo (sf.energy, juce::jlimit (0.0f, 1.0f, (f.intensity - 0.4f) * 1.7f + f.drop * 0.5f - f.breakdown * 0.6f), dt, 0.3f, 1.0f);
    sf.build   = f.build;
    sf.drop    = f.drop;
    sf.breakdown = f.breakdown;
    sf.kickHit = f.kickHit;
    sf.snareHit = f.snareHit;
    sf.dropHit = f.drop > 0.8f && lastDrop <= 0.8f;
    lastDrop = f.drop;

    // clocks: evolution (frozen by LOCK) and free motion (always runs, for breathing light)
    if (! set.locked)
        evo += dt * set.speed * (1.0f + 0.5f * sf.energy);
    motion += dt * set.speed;

    sf.evo = evo;
    sf.motion = motion;
    sf.beats = colours.beats();
    sf.beatValid = colours.beatValid();
    sf.beatPhase = colours.beatPhase();
    sf.barPhase = colours.barPhase();
    sf.beatsPerBar = colours.beatsPerBar();
    sf.speed = set.speed;
    sf.react = react;
    sf.locked = set.locked;
    sf.strip = set.strip;
    sf.quality = set.quality;
    sf.time = time;
    sf.dt = dt;
    sf.area = area;
    sf.pal = colours.palette();

    // the waveform
    const int displayLen = (int) juce::jlimit (256.0, 8192.0, sr * (set.strip ? 0.05 : 0.04));
    extractTriggered (h, f.scopeGain, f.peak, sr, displayLen);
    deriveShapes (dt);
    first = false;

    // worlds cross-fade (0.6 s); every world keeps its state so returning to one is seamless
    const int active = juce::jlimit (0, 2, (int) set.world);
    for (int i = 0; i < 3; ++i)
    {
        const float target = i == active ? 1.0f : 0.0f;
        weight[(size_t) i] = target > weight[(size_t) i] ? juce::jmin (target, weight[(size_t) i] + dt / 0.6f)
                                                         : juce::jmax (target, weight[(size_t) i] - dt / 0.6f);
    }

    if (weight[0] > 0.0f || active == 0) sacred.update (sf);
    if (weight[1] > 0.0f || active == 1) dream.update (sf);
    if (weight[2] > 0.0f || active == 2) signalWorld.update (sf);
    alignment.update (sf, set.alignment);
}

void TotemEngine::draw (juce::Graphics& g) const
{
    if (sf.area.getWidth() < 8.0f || sf.area.getHeight() < 8.0f)
        return;

    const float dim = 1.0f - 0.55f * alignment.amount();

    // draw the fading-out world first, the arriving one on top
    if (weight[1] > 0.0f) dream.draw (g, sf, weight[1] * dim);
    if (weight[2] > 0.0f) signalWorld.draw (g, sf, weight[2] * dim);
    if (weight[0] > 0.0f) sacred.draw (g, sf, weight[0] * dim);

    alignment.draw (g, sf);
}
}

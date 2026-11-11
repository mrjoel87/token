#include "Alignment.h"
#include "../Util.h"

namespace token::engine
{
void AlignmentDirector::trigger (const SignalFrame& s)
{
    shape = s.slow;
    age = 0.0f;
    cooldown = 45.0f;
    form = (form + 1) % 4;
}

void AlignmentDirector::update (const SignalFrame& s, bool enabled)
{
    const float dt = s.dt;

    // track calm stretches that follow real activity
    if (s.energy > 0.35f) { busyTimer += dt; calmTimer = 0.0f; }
    else if (s.energy < 0.15f && s.quiet < 0.9f) calmTimer += dt;

    if (age >= 0.0f)
    {
        if (! s.locked || age < rise)   // LOCK holds a formation that is already standing
            age += dt;
        else
            age = juce::jmin (age, rise + hold);
        if (age >= rise + hold + fall)
            age = -1.0f;
    }
    else
    {
        cooldown = juce::jmax (0.0f, cooldown - dt);
    }

    // envelope
    float target = 0.0f;
    if (age >= 0.0f)
    {
        if (age < rise)               target = fx::ease (age / rise);
        else if (age < rise + hold)   target = 1.0f;
        else                          target = 1.0f - fx::ease ((age - rise - hold) / fall);
    }
    level = target;

    // decide whether a moment is earned
    const bool dropEdge = s.dropHit || (s.drop > 0.6f && lastDrop <= 0.6f);
    lastDrop = s.drop;

    bool newPhrase = false;
    if (s.beatValid)
    {
        const auto phrase = (long long) std::floor (s.beats / (s.beatsPerBar * 16.0));
        newPhrase = lastPhrase >= 0 && phrase != lastPhrase;
        lastPhrase = phrase;
    }
    else
    {
        lastPhrase = -1;
    }

    if (! enabled || s.locked || age >= 0.0f || cooldown > 0.0f || s.quiet > 0.95f)
        return;

    float chance = 0.0f;
    if (dropEdge)                                    chance = 0.7f;
    else if (s.kickHit && s.build > 0.55f)           chance = 0.25f;
    else if (calmTimer > 10.0f && busyTimer > 20.0f) { chance = 0.5f; calmTimer = 0.0f; busyTimer = 0.0f; }
    else if (newPhrase && s.energy > 0.45f)          chance = 0.3f;

    if (chance > 0.0f && rng.nextFloat() < chance)
        trigger (s);
    else if (chance > 0.0f)
        cooldown = 6.0f;   // a missed chance still waits a little before the next one
}

void AlignmentDirector::draw (juce::Graphics& g, const SignalFrame& s) const
{
    if (level <= 0.005f)
        return;

    const auto A = s.area;
    const juce::Point<float> c = A.getCentre();
    const float R = juce::jmin (A.getHeight() * 0.44f, A.getWidth() * 0.3f) * (0.92f + 0.08f * level);

    // a soft dark well so the formation reads cleanly over the world
    fx::halo (g, c, R * 1.5f, juce::Colours::black.withAlpha (0.55f * level));
    fx::halo (g, c, R * 1.1f, s.pal.b.withAlpha (0.06f * level * (1.0f + s.bass)));

    switch (form)
    {
        case 0:  drawMandala   (g, s, c, R, level); break;
        case 1:  drawRune      (g, s, c, R, level); break;
        case 2:  drawEye       (g, s, c, R, level); break;
        default: drawInterlock (g, s, c, R, level); break;
    }
}

void AlignmentDirector::drawMandala (juce::Graphics& g, const SignalFrame& s, juce::Point<float> c, float R, float a) const
{
    const auto& pal = s.pal;
    const int q = s.quality;
    const float rot = s.motion * 0.03f;
    const float pulse = 1.0f + 0.08f * s.kick;

    // three concentric rings of the frozen waveform, folded 12 / 8 / 6 ways
    const int folds[] = { 12, 8, 6 };
    const float radii[] = { 0.95f, 0.68f, 0.42f };
    for (int ring = 0; ring < 3; ++ring)
    {
        path.clear();
        const int k = folds[ring];
        const int M = k * (q >= 2 ? 32 : 18);
        const float r0 = R * radii[ring] * pulse * (0.85f + 0.15f * a);
        for (int j = 0; j <= M; ++j)
        {
            const float th = (float) j / (float) M * fx::twoPi;
            const float v = fx::sampleAt (shape, fx::foldU (th, k));
            const auto p = fx::onCircle (c, r0 + v * R * 0.09f, th + rot * (ring % 2 == 0 ? 1.0f : -1.0f));
            if (j == 0) path.startNewSubPath (p); else path.lineTo (p);
        }
        path.closeSubPath();
        const auto col = ring == 1 ? pal.c : pal.a;
        fx::glow (g, path, col.withAlpha (0.9f * a), pal.b.withAlpha (a), 1.2f, 0.7f + 0.3f * s.bass, q);
    }

    // star lattice
    path.clear();
    fx::star (path, c, 12, 5, R * 0.95f * pulse, -rot);
    fx::star (path, c, 8, 3, R * 0.68f * pulse, rot);
    g.setColour (pal.b.withAlpha (0.4f * a));
    g.strokePath (path, fx::pen (0.8f));

    fx::halo (g, c, R * 0.18f * (1.0f + s.kick), pal.c.withAlpha (0.5f * a));
}

void AlignmentDirector::drawRune (juce::Graphics& g, const SignalFrame& s, juce::Point<float> c, float R, float a) const
{
    const auto& pal = s.pal;
    const int q = s.quality;
    const float H = R * 1.7f * (0.9f + 0.1f * a);
    const float top = c.y - H * 0.5f;

    // a central stave with branches read from the frozen waveform
    path.clear();
    path.startNewSubPath (c.x, top);
    path.lineTo (c.x, top + H);
    const int branches = 7;
    for (int b = 0; b < branches; ++b)
    {
        const float v = fx::sampleAt (shape, ((float) b + 0.5f) / (float) branches);
        const float y = top + H * (0.12f + 0.76f * (float) b / (float) (branches - 1));
        const float len = R * (0.18f + 0.32f * std::abs (v));
        if (v >= 0.0f || b % 3 == 0) { path.startNewSubPath (c.x, y); path.lineTo (c.x + len, y - len * 0.6f); }
        if (v <= 0.0f || b % 3 == 0) { path.startNewSubPath (c.x, y); path.lineTo (c.x - len, y - len * 0.6f); }
    }

    // a bounding diamond
    path.startNewSubPath (c.x, top - R * 0.08f);
    path.lineTo (c.x + R * 0.62f, c.y);
    path.lineTo (c.x, top + H + R * 0.08f);
    path.lineTo (c.x - R * 0.62f, c.y);
    path.closeSubPath();

    fx::glow (g, path, pal.c.withAlpha (0.92f * a), pal.c.withAlpha (a), 1.6f, 0.8f + 0.4f * s.snare, q);

    // orbiting points on the beat
    for (int i = 0; i < 6; ++i)
    {
        const auto p = fx::onCircle (c, R * 0.9f, s.motion * 0.12f + (float) i / 6.0f * fx::twoPi);
        const float size = 2.0f + 2.5f * s.kick;
        g.setColour (pal.a.withAlpha (0.7f * a));
        g.fillEllipse (p.x - size * 0.5f, p.y - size * 0.5f, size, size);
    }
}

void AlignmentDirector::drawEye (juce::Graphics& g, const SignalFrame& s, juce::Point<float> c, float R, float a) const
{
    const auto& pal = s.pal;
    const int q = s.quality;
    const float W = R * 1.25f, Hh = R * 0.55f * (0.3f + 0.7f * a);   // the eye opens as it rises

    // lids follow the frozen waveform
    path.clear();
    const int M = q >= 2 ? 120 : 70;
    for (int lid = 0; lid < 2; ++lid)
    {
        const float sign = lid == 0 ? -1.0f : 1.0f;
        for (int j = 0; j <= M; ++j)
        {
            const float u = (float) j / (float) M;
            const float x = c.x - W + u * W * 2.0f;
            const float env = std::sin (fx::pi * u);
            const float y = c.y + sign * Hh * env * (1.0f + 0.15f * fx::sampleAt (shape, u));
            if (j == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
        }
    }
    fx::glow (g, path, pal.a.withAlpha (0.95f * a), pal.b.withAlpha (a), 1.4f, 0.8f, q);

    // iris: a ring of the live waveform; pupil pulses with the kick
    path.clear();
    const float ir = Hh * 0.8f;
    const int K = q >= 2 ? 160 : 90;
    for (int j = 0; j <= K; ++j)
    {
        const float th = (float) j / (float) K * fx::twoPi;
        const auto p = fx::onCircle (c, ir + fx::sampleAt (s.low, fx::foldU (th, 9)) * ir * 0.18f, th);
        if (j == 0) path.startNewSubPath (p); else path.lineTo (p);
    }
    path.closeSubPath();
    fx::glow (g, path, pal.c.withAlpha (0.9f * a), pal.c.withAlpha (a), 1.2f, 0.6f + s.bass, q);

    const float pr = ir * (0.32f + 0.15f * s.kick);
    g.setColour (pal.c.withAlpha (0.85f * a));
    g.fillEllipse (c.x - pr, c.y - pr, pr * 2.0f, pr * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.9f * a));
    g.fillEllipse (c.x - pr * 0.55f, c.y - pr * 0.55f, pr * 1.1f, pr * 1.1f);

    // rays
    path.clear();
    for (int i = 0; i < 16; ++i)
    {
        const float an = (float) i / 16.0f * fx::twoPi;
        path.startNewSubPath (fx::onCircle (c, R * 0.98f, an));
        path.lineTo (fx::onCircle (c, R * (1.05f + 0.05f * s.high + (i % 2 == 0 ? 0.06f : 0.0f)), an));
    }
    g.setColour (pal.b.withAlpha (0.5f * a));
    g.strokePath (path, fx::pen (0.9f));
}

void AlignmentDirector::drawInterlock (juce::Graphics& g, const SignalFrame& s, juce::Point<float> c, float R, float a) const
{
    const auto& pal = s.pal;
    const int q = s.quality;
    const float rot = s.motion * 0.02f;
    const float r = R * 0.5f;
    const float spread = R * 0.5f * (0.4f + 0.6f * a);   // the circles drift together as it forms

    path.clear();
    const int M = q >= 2 ? 140 : 80;
    for (int i = 0; i < 6; ++i)
    {
        const auto cc = fx::onCircle (c, spread, rot + (float) i / 6.0f * fx::twoPi);
        for (int j = 0; j <= M; ++j)
        {
            const float th = (float) j / (float) M * fx::twoPi;
            const float v = fx::sampleAt (shape, fx::foldU (th + (float) i, 3));
            const auto p = fx::onCircle (cc, r * (1.0f + 0.05f * v), th);
            if (j == 0) path.startNewSubPath (p); else path.lineTo (p);
        }
        path.closeSubPath();
    }
    fx::glow (g, path, pal.a.withAlpha (0.85f * a), pal.b.withAlpha (a), 1.1f, 0.7f, q);

    path.clear();
    path.addEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    fx::glow (g, path, pal.c.withAlpha (0.8f * a), pal.c.withAlpha (a), 1.3f, 0.5f + s.bass, q);

    fx::halo (g, c, R * 0.25f * (1.0f + s.kick), pal.c.withAlpha (0.45f * a));
}
}

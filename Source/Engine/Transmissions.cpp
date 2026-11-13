#include "Transmissions.h"
#include "../UI/TotemLookAndFeel.h"
#include <cstring>

namespace token::engine
{
namespace
{
    const char* const phrases[] = {
        "CAN YOU HEAR IT",
        "THE SIGNAL REMEMBERS YOU",
        "WE ARE NOT ALONE IN HERE",
        "DO NOT LOOK AT THE CENTRE",
        "FREQUENCY ACCEPTED",
        "IT KNOWS THE BEAT",
        "YOU HAVE BEEN HERE BEFORE",
        "THE DOOR IS OPEN",
        "ALL IS VIBRATION",
        "STAY IN THE LOOP",
        "TRANSMISSION 7 . 7 . 3",
        "WAKE UP",
        "THEY ARE DANCING TOO",
        "THIS IS NOT THE FIRST ROOM",
        "TURN IT UP",
        "THE WALLS ARE BREATHING",
        "SIGNAL FROM THE OTHER SIDE",
        "NOTHING IS SILENT",
        "COUNT THE CIRCLES",
        "HOLD ON TO THE BASS",
    };
    constexpr int numPhrases = (int) (sizeof (phrases) / sizeof (phrases[0]));

    const char* const scramble = "#%&@*+=?/<>{}[]0123456789ABCDEFGHJKLMNPQRSTUVWXYZ";
}

Transmissions::Transmissions()
{
    rng.setSeedRandomly();
    gap = 18.0f + rng.nextFloat() * 20.0f;
    station = "00 00 00";
}

void Transmissions::begin (const SignalFrame&)
{
    int pick = rng.nextInt (numPhrases);
    if (pick == lastPhrase)
        pick = (pick + 1) % numPhrases;
    lastPhrase = pick;
    message = phrases[pick];
    age = 0.0f;
    life = 5.5f + 0.08f * (float) message.length();
}

void Transmissions::update (const SignalFrame& s)
{
    // the corner readout changes on beats (or kicks without a host)
    stationClock += s.dt;
    if ((s.beatValid && s.beatPhase < 0.05f && stationClock > 0.3f) || (! s.beatValid && s.kickHit))
    {
        stationClock = 0.0f;
        station = juce::String::formatted ("%02d %02d %02d", rng.nextInt (100), rng.nextInt (100), rng.nextInt (100));
    }

    if (age >= 0.0f)
    {
        age += s.dt;
        if (age >= life)
        {
            age = -1.0f;
            gap = 35.0f + rng.nextFloat() * 50.0f;   // rare: they should feel like something got through
        }
        return;
    }

    if (s.locked)
        return;

    gap -= s.dt * juce::jmax (0.5f, s.speed);
    if (gap <= 0.0f || (s.dropHit && gap < 15.0f))
        begin (s);
}

void Transmissions::rebuildText (const SignalFrame& s, const juce::String& text) const
{
    const auto A = s.area;
    const float h = juce::jlimit (14.0f, 64.0f, (s.strip ? A.getHeight() * 0.16f : A.getHeight() * 0.07f));
    const auto font = ui::font (h, "Light", 0.35f);

    juce::GlyphArrangement ga;
    ga.addLineOfText (font, text, 0.0f, 0.0f);
    juce::Path textPath;
    ga.createPath (textPath);
    const auto tb = textPath.getBounds();
    const float y = s.strip ? A.getBottom() - h * 0.6f : A.getBottom() - A.getHeight() * 0.1f;
    textPath.applyTransform (juce::AffineTransform::translation (A.getCentreX() - tb.getCentreX(), y - tb.getCentreY()));

    letters.clear();
    juce::PathFlatteningIterator it (textPath, {}, 0.6f);
    bool newSub = true;
    while (it.next())
    {
        if (newSub) letters.moveTo (it.x1, it.y1);
        letters.lineTo (it.x2, it.y2);
        newSub = it.closesSubPath || it.isLastInSubpath();
    }
}

void Transmissions::draw (juce::Graphics& g, const SignalFrame& s, juce::Colour ink, juce::Colour glow, juce::Colour dim) const
{
    // numbers station: tiny, low, in the corner
    {
        const auto A = s.area;
        g.setFont (ui::font (9.0f, "SemiBold", 0.5f));
        g.setColour (dim.withAlpha (0.35f + 0.25f * s.kick));
        g.drawText (station, juce::Rectangle<float> (A.getRight() - 90.0f, A.getBottom() - 14.0f, 86.0f, 12.0f),
                    juce::Justification::centredRight, false);
    }

    if (age < 0.0f)
        return;

    // decode: characters lock in one by one, the rest are noise; it breaks apart at the end
    const int n = message.length();
    const float decode = juce::jlimit (0.0f, 1.0f, age / 1.6f);
    const float fadeOut = juce::jlimit (0.0f, 1.0f, (life - age) / 1.2f);
    const int locked = (int) (decode * (float) n);

    juce::String shown;
    juce::Random noise ((juce::int64) (s.time * 24.0f));
    for (int i = 0; i < n; ++i)
    {
        const juce::juce_wchar ch = message[i];
        const bool broken = fadeOut < 1.0f && noise.nextFloat() > fadeOut;
        if (ch == ' ' || (i < locked && ! broken))
            shown << juce::String::charToString (ch);
        else
            shown << juce::String::charToString ((juce::juce_wchar) scramble[noise.nextInt ((int) std::strlen (scramble))]);
    }

    if (shown != builtFor || s.area != builtArea)
    {
        rebuildText (s, shown);
        builtFor = shown;
        builtArea = s.area;
    }

    PenStyle st;
    st.amp = 1.5f + 3.0f * s.bass + 2.0f * s.kick;
    st.cycles = 18.0f;
    st.phase = s.motion * 0.2f;
    st.stepPx = 2.0f;
    st.maxPoints = s.quality >= 2 ? 2400 : 1400;
    st.useFine = false;
    pen.build (letters, {}, st, s, path);

    const float a = juce::jmin (1.0f, age / 0.3f) * fadeOut;
    ScopePen::glow (g, path, ink.withAlpha (0.95f * a), glow.withAlpha (a), 1.1f, 0.9f, s.quality);
}
}

# SMSW TOTEM 2

An audio-reactive visualiser for Windows / Ableton Live (VST3, JUCE 8), built to be projected.
**Everything on screen is the oscilloscope**: every symbol, shape and letter is drawn with a line
that is the live waveform. Audio passes through untouched.

*The plugin keeps its internal identity (manufacturer code `Smsw`, plugin code `Tokn`, state tag
`TOKEN`) so existing Live Sets still find it. Only the visible name changed.*

## What you see

- **The spine**: the plain scope trace across the screen. Everything grows from it.
- **Breathing rings** around the centre that swell with the volume and kick.
- **A summoned image** at the centre: one of 44, drawn on line by line, held, then dissolved into
  noise as the next arrives. A shuffle makes sure every image appears before any repeats.
- **Pop-ups**: smaller images appear around the main one for a few seconds.
- **Messages from nowhere**: now and then a line of text decodes out of scrambled characters,
  holds, and breaks apart. A tiny numbers readout flickers in the corner on beats.

### The 44 images

| Realm | Images |
|---|---|
| Occult (15) | pentagram, hexagram, heptagram, octagram, enneagram, eye in the triangle, chaos star, the four elements, sulfur, mercury, ankh, sigil (different every time), rune stave (different every time), magic square, ouroboros |
| Sacred geometry (15) | seed of life, flower of life, Metatron's cube, vesica piscis, triquetra, Borromean rings, golden spiral, sun wheel, crescent and star, yantra, lotus, labyrinth, spirograph, Lissajous knot, pyramid of light |
| Strange (4) | the watcher (blinks on kicks), three-eyed face, hourglass, orrery |
| Interdimensional (10) | tesseract and 16-cell (true 4D rotation), hypersphere (Hopf fibres), merkaba, icosahedron, torus knot, torus field, portal, infinite corridor, rift |

## How it reacts

| Music | Response |
|---|---|
| Volume | everything breathes: scale, brightness, the spine |
| Kick | pulse, flash, the watcher blinks, echoes brighten |
| Bass | how far the waveform pushes the lines; line weight |
| Snare | the lines split into colours for an instant |
| Energy | depth echoes: copies of the image fall away into the distance |
| Drop | the image shatters into a ring of copies and a new one is summoned |
| Every 4 bars (host playing) | the image flips through itself |
| Quiet | dims, stills, keeps breathing |

Tempo: Live's song position is used only while the transport plays. When stopped, the status shows
FREE and the visuals follow detected kicks instead of an invented beat.

## Controls

| Control | What it does |
|---|---|
| **COLOUR** | ABYSS · UV · ACID · BLOOD · GOLD · PULSE (moves through the palettes with the music) |
| **THEME** | DARK (light on black, for projection) · LIGHT (ink on paper) |
| **LAYOUT** | STRIP or BOX; both freely resizable, each remembers its size |
| **REACT** | how strongly the music moves the visuals (double-click to reset) |
| **SPEED** | how fast images change and evolve (never touches audio or tempo) |
| **LOCK** | holds the current image and composition. It still breathes and reacts. |
| **CLEAN** | hides every control for projection / filming; double-click anywhere to toggle |

## Performance

Only the screen rectangle repaints each frame; no transparency layers or path clips; all buffers
are allocated once. Drawing time is measured and detail steps down automatically if it gets heavy
(fewer points, fewer echoes, fewer glow passes). The audio thread only copies samples into a
lock-free FIFO.

## Manual checks in Ableton Live 11 (please verify)

- [ ] Loads as "SMSW TOTEM 2"; an old Live Set with TOTEM on a track still opens it
- [ ] Audio passes through unchanged
- [ ] Smooth at full screen in BOX and in STRIP; resizing works in both
- [ ] COLOUR and THEME change everything, including in LIGHT
- [ ] LOCK holds the image while it keeps reacting
- [ ] Images change, pop-ups appear, a message arrives within a couple of minutes
- [ ] Transport stopped shows FREE; playing shows BPM and flips every 4 bars
- [ ] CLEAN hides controls; double-click toggles

## Build (GitHub, nothing to install)

The workflow in `.github/workflows` builds on GitHub's Windows machines. Download the artifact
from the Actions run (it contains `SMSW TOTEM 2.vst3`) and copy it to
`C:\Program Files\Common Files\VST3`. Remove any older `SMSW TOTEM.vst3` / `TOKEN.vst3` (same plugin ID).

## Architecture

```
Source/
  PluginProcessor     pass-through audio, lock-free sample FIFO, host transport atomics, state
  AudioAnalyser       FFT bands, kick / snare / hat onsets, build, drop, breakdown, loudness
  ColourEngine        palettes / PULSE, host beat clock (only when Live plays)
  Engine/
    SignalFrame.h     the per-frame musical snapshot
    Sketch.h          drawings as strokes in unit space
    ScopePen          turns any sketch into an oscilloscope trace
    Glyphs            the 44 images (2D symbols, 3D and 4D objects)
    Transmissions     messages from nowhere
    TotemEngine       waveform extraction, summoning, echoes, flip, shatter, glitch
  UI/                 slider look, pill selectors and toggles
  PluginEditor        layouts, themes, partial repaint, adaptive quality
```

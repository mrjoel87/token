# SMSW — TOKEN · The Living Oscilloscope (V1 source)

A VST3 audio visualiser for Windows / Ableton Live 11, built with JUCE 8 + CMake.
TOKEN passes audio through untouched and draws a real, sample-driven oscilloscope inside a
miniature living world.

> **Status:** source complete for Stages 1–3 (plus the rarity scheduler from Stage 4).
> It has **not been compiled or tested yet** — this machine had no C++ toolchain when it was written.
> Expect a short round of compile fixes on the first build.

## Build on GitHub (nothing to install)

1. Create a new repository on github.com. Upload the **contents** of this `TOKEN` folder so that
   `CMakeLists.txt` sits at the top of the repository (drag the `Source` folder and the files in).
2. Windows often skips the hidden `.github` folder when you drag files in. If it's missing, use
   *Add file → Create new file*, name it `.github/workflows/build.yml`, paste in the contents of
   `.github/workflows/build.yml` and commit.
3. Open the **Actions** tab. The build runs automatically (about 5–10 minutes).
4. When it shows a green tick, open the run and download **TOKEN-vst3** at the bottom. Unzip it
   and copy the `TOKEN.vst3` folder into `C:\Program Files\Common Files\VST3` (Windows asks for
   admin permission).
5. A red cross means a compile error: open the run, click the failed step and copy the error.

## Build locally

You need:

1. **Visual Studio 2022 Build Tools** with the *Desktop development with C++* workload
   (https://aka.ms/vs/17/release/vs_BuildTools.exe). You can install it to another drive;
   the Windows SDK part still goes on C: (~1.5–2 GB).
2. **CMake ≥ 3.22** (the portable zip is fine; put its `bin` folder on PATH).
3. **JUCE 8** — either let CMake download it automatically (default: tag `8.0.8`), or point to
   a local copy.

```powershell
cd TOKEN
.\build.ps1                                # downloads JUCE during configure
# or
.\build.ps1 -JucePath "I:\SMSW\tools\JUCE"
```

If the JUCE tag isn't available, use another JUCE 8 release: add `-DTOKEN_JUCE_VERSION=8.0.x`
to the configure step in `build.ps1`.

Install (Administrator PowerShell):

```powershell
.\install.ps1     # copies TOKEN.vst3 to C:\Program Files\Common Files\VST3
```

In Ableton Live 11: *Preferences → Plug-Ins → Use VST3 Plug-In System Folders: On → Rescan*.
TOKEN appears under *Plug-Ins → SMSW*. Drop it on any track (or the master) — it is an audio
effect that does not change the sound.

## Controls

| Control | What it does |
|---|---|
| **MODE** STRIP / BOX | Slim 1000×220 strip for the top/bottom of Live, or the 900×650 window. Each layout remembers its own size. |
| **REACT** | How strongly the world responds to audio (bass, transients, highs). |
| **CHAOS** | How often unusual events appear and how intense CHAOS gets. |
| **GLOW** | Luminosity **and** phosphor persistence (longer trails). |
| **SCENE** | AUTO (cycles through Dream, Mystic and Norse every ~1–2 min) · MIN · DREAM · MYSTIC · NORSE |
| **SCOPE** TRIG / SCROLL | Triggered standing wave, or a scrolling envelope of the last ~1 s. |
| **STEREO / AGC / SPEC** (Box) | Split L/R lanes · automatic visual gain · secondary spectrum. |
| **CLEAN** | Hides all hardware for filming. **Double-click anywhere** to toggle; in clean mode a hidden LED sits in the top-right corner. |

## Architecture

```
Source/
  PluginProcessor     transparent stereo pass-through, lock-free sample FIFO, state + window sizes
  ScopeBuffers.h      ScopeFifo (audio → UI, lock-free) and ScopeHistory (UI-side ring)
  AudioAnalyser       RMS/peak, 2048-pt FFT bands, spectral-flux onsets (kick-like / snare-like),
                      intensity, sustain, Dream/Pulse/Chaos state machine with hysteresis, auto gain
  Oscilloscope        zero-crossing triggered / scrolling scope, glow passes, phosphor trails,
                      smoothed wave + envelope for the world to ride on
  World/
    WorldEngine       themed skies, parallax skylines, stars, moon, aurora, shockwaves,
                      ambient particles, weighted-random event scheduler
    Events.cpp        the scenes (add new ones here + register in createEventLibrary)
    PixelArt          seam-free sprite renderer, glow strokes, Elder Futhark rune set
    Particles         fixed-pool particle system (no allocation per frame)
  UI/                 luxury LookAndFeel knobs, segment selectors, LED switches
  PluginEditor        layouts (Strip / Box / Clean), chassis, black-glass screen, frame loop
```

The audio thread only copies samples into a lock-free FIFO. All analysis and drawing happen on
the message thread, synced to the display's vertical blank (VBlankAttachment).

## Living world — V1 event library

| Rarity | Event | Behaviour |
|---|---|---|
| Common | Shooting star | More likely on snares |
| Common | Rune | A random Elder Futhark rune condenses out of noise, glows with bass, dissolves into sparks |
| Uncommon | Aeroplane | Flies just above the waveform peaks, beacon + strobe, contrail; often when music drops back to Dream |
| Uncommon | Rain cloud | Pixel cloud swells with bass, rain density follows the highs, lightning on kicks in CHAOS |
| Uncommon | Ravens | Cross the screen, often when leaving CHAOS |
| Rare | Bench couple | Two silhouettes under a flickering streetlight; favours quiet passages |
| Rare | Longship | Sails *on* the waveform, oars speed up with bass, a voyage rune glows above it |
| Rare | Eye & pyramid | Stepped pyramid rises; edges glow with bass; the eye opens, blinks, tracks snares, rays in CHAOS |
| Very rare | Whale | Rises from the bottom, spouts, sinks |
| Very rare | Portal | Rotating gold geometry swallowing particles |
| Secret | Creator cameo | Placeholder pixel musician at a tiny synth; keys light with the music |

Ambient layers are always present: twinkling stars that react to highs, a lonely house with a lit window
(Dream), distant pyramids (Mystic), snow-capped mountains + aurora that grows with sustained sound (Norse),
dust, snow, sand, chaos embers, sparkles riding the wave, kick shockwaves, a slow hexagram in CHAOS.

For repeatable debugging, enable `TOKEN_DEBUG_SEED` in `CMakeLists.txt`.

## Next stages

- **Stage 4:** more runes and monuments, Japanese imagery (torii, lanterns, pink moon temple, koi),
  Egyptian set (Eye of Horus, obelisks), celestial objects, more scenes.
- **Stage 5:** portrait-based creator sprite (load from an asset), more cameo variants, polish,
  performance profiling.

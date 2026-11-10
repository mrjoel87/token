# SMSW TOTEM

An audio-reactive visualiser for Windows / Ableton Live (VST3, JUCE 8). The artwork is generated
entirely by the incoming sound - no images, no pre-made animations. Audio passes through untouched.

*Formerly TOKEN. The plugin keeps TOKEN's internal identity (manufacturer code `Smsw`, plugin code
`Tokn`, state tag `TOKEN`) so Live Sets saved with TOKEN still find it - only the visible name changed.*

## Controls

| Control | Options |
|---|---|
| **VISUAL** | CLASSIC · ORBIT · MIRROR · NONE |
| **COLOUR** | MANUAL (fixed palette) · BEAT COLOUR (evolves with the host tempo) |
| **THEME** | DARK · LIGHT |
| **INTENSITY** | strength of all visual reactions (double-click to reset) |
| **LAYOUT** | STRIP (wide, 1000×220) · LARGE (920×640) - both resizable, each remembers its size |
| **CLEAN** | hides every control for filming; double-click anywhere to toggle. A small "SMSW TOTEM" signature stays in the corner. |

## Artwork (DISPLAY: SCOPE+ART · ART · SCOPE)

116 original artwork elements, drawn live in fine lines, silhouettes, soft glows and small pixel art,
coloured by the theme and palette (`Source/Art/`):

| Category | Count | Examples |
|---|---|---|
| Ancient & mysterious | 25 | Viking longship on a waveform sea, runestone, rune circle, pyramid and eye, torii, pagoda, ruins, Wedjat eye, ankh, scarab, ouroboros, valknut, triple spiral, labyrinth, alchemical signs |
| Nature & weather | 21 | whale, koi, jellyfish, birds, geese, ravens, rain cloud, storm, cirrus, snowy peaks, fog, pines, moon on the sea, cherry blossom, bamboo, fireflies, aurora, lotus, lone tree |
| Celestial | 18 | moons and phases, ringed planet, orrery, eclipse, comet, meteors, Orion / Plough / Cassiopeia / Northern Cross, galaxy, nebula, astrolabe, star chart, black hole |
| Sentimental | 19 | two on a bench, umbrella, little aeroplane, kite, beach walk, lighthouse, one lit window, night train, station, stargazer, rooftop cat, rain on a window, lantern boat, swing, telescope, a secret musician |
| Surreal | 17 | floating island, door in the sea, eye in the geometry, sky lanterns, endless stairs, moon on a string, hand from the sea, hourglass, portal, many eyes, number transmission, face in the static |
| Geometric | 16 | seed / flower of life, Metatron's cube, vesica, hexagram, icosahedron, torus, Lissajous, spirograph, golden spiral, ripples, tesseract, sunflower spiral, impossible triangle, standing waves |

Three pairs of elements share a drawing routine but are different pictures: the four constellations
(different star patterns) and the three sacred-geometry circle figures.

**Scheduler.** A shuffle bag guarantees every element appears once before any can return, and the
last 14 shown never start a new cycle. From the front of the bag, candidates are scored against a
slow musical profile (quiet / bass / melodic / energetic), so quiet passages bring sentimental
scenes and landscapes, bass-heavy passages bring weighty symbols, energetic passages bring geometry
and drama. At most one main element plus one or two small companions (birds, planes, comets,
constellations, lanterns) are shown at once. Durations range from ~8 s to ~30 s, with breathing room
between pieces (longer in breakdowns). Drops end the current piece early and bring in something
more dramatic. Placement, scale, mirroring, drift and entrance (fade, wipe, iris, rise) vary each time.

**DISPLAY.** SCOPE+ART draws the artwork behind the oscilloscope; ART hides the scope and the
spectrum completely (fading out smoothly) and gives the artwork the whole screen; SCOPE turns the
artwork off.

## Visual modes

- **Classic** - zero-crossing triggered horizontal oscilloscope with layered trails.
- **Orbit** - the waveform wrapped around a circle that breathes, deforms and rotates; echoes expand outwards.
- **Mirror** - a phase-space portrait of the signal (sample vs. delayed sample) mirrored into a kaleidoscope.
- **None** - the oscilloscope is hidden and the spectrum fills the screen.

## How it listens

| Musical feature | Detected from | Visual response |
|---|---|---|
| Kick drums | low-band spectral flux onsets | controlled pulse / expansion, soft bloom |
| Basslines | bass level (absolute + relative to its own recent peak) | line weight, Orbit radius and deformation |
| Pads / sustained sound | loud but few onsets | longer trails, slower, more fluid motion |
| Hi-hats / percussion | 7-16 kHz flux onsets + relative highs | delicate fast detail line / ring |
| Mids | mid-band level | Orbit / Mirror rotation speed |
| Drops | sudden rise of energy above its slow average | line splits into several luminous lines; Orbit gains 3-fold symmetry; Mirror grows to 6-fold |
| Breakdowns | energy falling away from a louder section | thinner, calmer, simpler display |

Sensitivity is adaptive: each band is compared with its own recent maximum, so quiet mixes still move
the visuals and loud mixes don't saturate them.

**Beat Colour** reads Ableton's tempo, time signature and song position. It pulses gently on each
beat (a little more on the downbeat and in loud passages, less in breakdowns) and crossfades to the
next palette over 1, 2 or 4 bars depending on the energy. When the transport is stopped it keeps
time from the host BPM.

## Build (GitHub, nothing to install)

Upload the contents of this folder to a GitHub repository. The workflow in `.github/workflows`
builds it on GitHub's Windows machines; download **SMSW-TOTEM-vst3** from the Actions run.

Local build (needs VS 2022 Build Tools + CMake): `.\build.ps1`, then `.\install.ps1` as Administrator
(it also removes the old `TOKEN.vst3`, since both share one plugin ID).

## Architecture

```
Source/
  PluginProcessor   pass-through audio, lock-free sample FIFO, host tempo/transport atomics, state
  ScopeBuffers.h    audio -> UI FIFO and UI-side sample history
  AudioAnalyser     FFT bands (absolute + adaptive), kick / snare / hat onsets, intensity, drops, breakdowns
  ColourEngine      MANUAL palette or BEAT COLOUR (tempo-synced palette evolution + beat pulse)
  Theme             DARK / LIGHT interface looks and the artwork palettes
  Visualiser        Classic / Orbit / Mirror / None + spectrum
  UI/               slider look, pill selectors and toggles
  PluginEditor      Strip / Large / Clean layouts, wordmark, frame loop (vblank-synced, message thread)
```

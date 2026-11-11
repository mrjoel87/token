# SMSW TOTEM 2.0

An audio-reactive visualiser for Windows / Ableton Live (VST3, JUCE 8). Every image is generated
from the incoming sound: the oscilloscope signal is the raw material of all three worlds.
Audio passes through untouched.

*Formerly TOKEN. The plugin keeps its internal identity (manufacturer code `Smsw`, plugin code
`Tokn`, state tag `TOKEN`) so existing Live Sets still find it.*

## Controls

| Control | What it does |
|---|---|
| **SCENE** | SACRED · DREAM · SIGNAL (cross-fade over 0.6 s; each world keeps its state) |
| **COLOUR** | ABYSS (cyan / electric blue / amber) · UV (lilac / ultraviolet / cyan) · AMBER · IVORY · PULSE (moves through the palettes with the music) |
| **REACT** | how strongly the music moves the visuals (double-click to reset) |
| **SPEED** | how fast the composition evolves, 0.3x to 2.5x (never touches audio or tempo) |
| **LOCK** | freezes the composition (symmetry, glyphs, ribbon history, DREAM moment). Audio reactions continue. |
| **ALIGN** | allows the rare ALIGNMENT moments (on by default) |
| **LAYOUT** | STRIP or BOX; both resizable, each remembers its size |
| **CLEAN** | hides every control for filming; double-click anywhere to toggle. A small signature stays. |

The status line shows scene / colour / HELD, and on the right either the host BPM (Live playing)
or FREE (no host beat grid; nothing is beat-synced then, only onset-driven).

## The three worlds

**SACRED**: the waveform folded into k-fold symmetry (8, 6, 12, 7, 10, 5, 9 folds, a new one every
8 bars with the host or ~24 s without). A bass ring of the low-passed waveform; petals whose edges are
the waveform (unfolding with sustained sound, veined by hi-hats); a polygon and star frame that steps
on host beats (or real kicks without a host) and brightens through build-ups; rune spokes whose
branches are read from the waveform every 4 bars and flash on snares; a core that pulses with kicks
and blooms on drops. In STRIP it becomes a frieze of counter-rotating rosettes sized to the height.

**DREAM**: a night horizon made of the waveform (hills breathe with the bass) with a faint
reflection, glints of high detail on the ridge, a slow moon, stars twinkling with hats, clouds that
thicken with pads. Rarely, one small pixel-art moment appears (two on a bench, a little aeroplane, a
rain cloud, one lit window, a lantern boat, a night train, a lighthouse, geese, a comet...), drawn from
a shuffled list so nothing repeats until all have appeared. One at a time, with long gaps.

**SIGNAL**: the premium scope. The live, zero-crossing-triggered trace (bass weight, kick pulse); its
recent history as ribbons of light (a waveform tunnel receding to a vanishing point in BOX, a rising
ribbon in STRIP); snares split the trace into two fine coloured echoes; a restrained spectrum floor.

**ALIGNMENT**: rare moments where everything settles into one formation built from a snapshot of the
waveform: a mandala, a rune structure, a geometric eye or interlocking circles. They rise (1.4 s), hold
(2.8 s) and dissolve (2.4 s). They are only a chance on a drop, a kick at the top of a build-up, a calm
after activity or a 16-bar phrase. There is a 45 s cooldown and nothing in the first 20 s.

## How it listens

| Music | Detected from | Response |
|---|---|---|
| Kick | low-band spectral-flux onsets, adaptive threshold | pulse, weight, core flash |
| Bass | absolute + relative bass level | ring size, horizon height, line weight |
| Snare | mid-band onsets | brief spokes / echo split |
| Pads, sustained sound | loud with few onsets | slow unfolding, lazier waveform, clouds |
| Hi-hats, highs | 7-16 kHz flux + relative highs | fine veins, dust, star twinkle, glints |
| Quiet passages | silence / breakdown / loudness | stillness, dimming, breathing |
| Build-ups | intensity climbing above its 20 s average | faster rotation, brighter frame, extra polygon |
| Drops | sudden energy rise | bloom, alignment chance |

Tempo: Live's song position is used only while the transport plays. When it is stopped there is no
invented beat grid; the visuals follow detected onsets and envelopes instead.

## Performance (2.0 diagnosis and fixes)

Diagnosed from the 1.x code (by inspection, not a profiler):

1. **Full-window repaint every vblank**: the chassis, wordmark and all controls were redrawn 60x/s.
   Fixed: only the screen rectangle repaints per frame; chrome repaints only on change.
2. **Transparency layers per art element and for scope fades**: very expensive in Direct2D.
   Fixed: removed; one small clipped layer only for the single DREAM moment.
3. **Path clip regions** (rounded screen clip, waveform "reveal" region, drawing the art twice).
   Fixed: rectangle clip only; rounded corners are painted over with a cached mask.
4. **Multi-pass glow** (up to dozens of wide strokes in Mirror). Fixed: core + at most 2 halo passes.
5. **Per-frame allocations** (Path copies for trails, vectors resized). Fixed: reused member Paths
   and fixed-size arrays; scratch buffers allocated once.
6. **Grain texture + full-screen vignette gradient every frame**. Fixed: grain removed, vignette
   rendered once per size into an image.
7. **Adaptive quality**: the engine's drawing time is measured. Above ~7 ms the quality steps down
   (2 to 1 to 0: fewer points, fewer halo passes, fewer layers). It steps back up after 5 s of easy frames.

The audio thread only copies samples into a lock-free FIFO and stores transport values in atomics.

## Manual checks in Ableton Live 11 (not yet done; please verify)

- [ ] Plugin loads as "SMSW TOTEM" and an old Live Set with TOKEN/TOTEM on a track still opens it
- [ ] Audio passes through unchanged (null test against a bypassed copy)
- [ ] Smooth motion in BOX at full screen and in STRIP with a busy track (no stutter when resizing)
- [ ] SCENE switches cross-fade smoothly; COLOUR palettes and PULSE change with the music
- [ ] Transport stopped: status shows FREE and nothing jumps on a fake beat
- [ ] Transport playing: BPM shown, SACRED steps on beats, symmetry changes every 8 bars
- [ ] LOCK holds the composition while kicks / bass still move it
- [ ] ALIGNMENT appears rarely (drop / build); ALIGN off stops it completely
- [ ] STRIP at 140-180 px height uses the two-column panel; nothing clipped or stretched
- [ ] CLEAN hides all controls; double-click toggles; settings saved with the Live Set
- [ ] CPU in Live's meter stays low with the editor closed (analysis runs only while it is open)

## Build (GitHub, nothing to install)

The workflow in `.github/workflows` builds on GitHub's Windows machines. Download the artifact
from the Actions run (it contains `SMSW TOTEM.vst3`) and copy it to
`C:\Program Files\Common Files\VST3` (remove any old `TOKEN.vst3`, same plugin ID).

## Architecture

```
Source/
  PluginProcessor     pass-through audio, lock-free sample FIFO, host transport atomics, state
  ScopeBuffers.h      audio -> UI FIFO and UI-side sample history
  AudioAnalyser       FFT bands (absolute + adaptive), kick / snare / hat onsets, build, drop, breakdown
  ColourEngine        studio palettes / PULSE, host beat clock (only when Live plays)
  Engine/
    SignalFrame.h     the per-frame musical snapshot + lean drawing helpers
    TotemEngine       waveform extraction (triggered, low / fine / slow), clocks, world cross-fade
    SacredWorld       symmetry, petals, runes
    DreamWorld        waveform horizon + rare pixel moments (uses Art/ library)
    SignalWorld       trace, ribbons, tunnel, spectrum floor
    Alignment         rare formations with cooldown
  Art/                pixel-art / line-art library (DREAM uses a sentimental subset)
  UI/                 slider look, pill selectors and toggles
  PluginEditor        layouts, partial repaint, cached vignette, adaptive quality, HUD
```

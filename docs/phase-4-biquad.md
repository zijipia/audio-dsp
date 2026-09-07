# Phase 4 — Biquad filters

Status: **implemented on `main`**

## Completed

- [x] Biquad abstraction in the native DSP context.
- [x] Low-pass, high-pass, band-pass, and notch filters.
- [x] Peaking EQ, low shelf, and high shelf.
- [x] RBJ-style coefficient calculation with normalization by `a0`.
- [x] Runtime coefficient updates through `setBiquad()` without rebuilding the DSP instance.
- [x] Independent delay-line state per channel.
- [x] `clearBiquad()` for returning to transparent processing.
- [x] `reset()` clears biquad state and restores the Phase 3 defaults.
- [x] Parameter validation: frequency below Nyquist, positive Q, gain limited to -24..24 dB.
- [x] Frequency-response and lifecycle-oriented tests.

## API

```ts
dsp.setBiquad({ type: "lowPass", frequency: 2000, q: 0.707 });
dsp.clearBiquad();
```

Supported types: `lowPass`, `highPass`, `bandPass`, `notch`, `peaking`, `lowShelf`, `highShelf`.

`q` defaults to `1 / sqrt(2)` and `gain` defaults to `0 dB` when omitted. Gain is used by peaking and shelf filters.

## Processing model

The miniaudio converter remains responsible for the native PCM block boundary. The biquad runs over the converted block in native code, after volume/mute/pan. State is preserved between `process()` calls. Each channel has independent delay-line state.

For `s16`, samples are processed in normalized floating-point form and converted back with output clipping. `f32` samples are processed directly and clipped to `[-1, 1]`.

No audio device I/O is introduced by Phase 4.

## Version

Package version: `0.5.0`

Native addon version: `0.5.0-phase4-biquad`

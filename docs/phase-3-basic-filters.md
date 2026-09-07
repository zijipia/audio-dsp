# Phase 3 — Basic filters

Status: **implemented on `main`**

## Completed

- [x] Volume / gain with runtime updates.
- [x] Mute with runtime updates.
- [x] Linear stereo pan/balance with runtime updates.
- [x] Basic clipping protection for `s16` and `f32` output.
- [x] Parameter validation and safe ranges.
- [x] Reset restores neutral filter parameters.
- [x] Tests for filter behavior, validation, reset, and lifecycle.

## API

`AudioDSP` now exposes:

```ts
dsp.setVolume(volume: number): void;
dsp.setMute(muted: boolean): void;
dsp.setPan(pan: number): void;
```

The processing instance is not rebuilt when these parameters change.

## Semantics

- Volume range: `0..4`, with `1` as unity gain.
- Mute: `true` produces silence; `false` restores the configured volume.
- Pan: `-1` is full left, `0` is center, `+1` is full right. Pan requires at least two channels.
- For stereo, pan affects the first two interleaved channels; additional channels receive the configured volume gain.
- `s16` output is clamped to `[-32768, 32767]`.
- `f32` output is clamped to `[-1, 1]`.
- `reset()` reinitializes the miniaudio converter and restores volume `1`, mute `false`, and pan `0`.

The existing Phase 2 miniaudio data-converter path remains the native PCM block boundary; basic filters are applied to the returned native output buffer before it crosses back to JavaScript.

## Non-goals

Phase 3 does not add biquad filters, EQ, limiter/compressor dynamics, or a mutable filter graph. Those remain later roadmap phases.

## Version

Package version: `0.4.0`

Native addon version: `0.4.0-phase3-basic-filters`

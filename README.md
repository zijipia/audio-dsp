# @ziji/audio-dsp

Native DSP engine for ZiPlayer.

`@ziji/audio-dsp` provides low-overhead native PCM processing for ZiPlayer. It focuses on real-time DSP rather than decoding media, hardware playback, or replacing FFmpeg.

## Architecture

```text
PCM -> Volume/Pan -> Filter Graph -> Compressor -> Limiter -> Soft Clip -> PCM
```

The graph and dynamics state are owned by one DSP instance. `process()` and mutation APIs are synchronous; callers must use a single owner and must not invoke the same DSP instance concurrently from multiple worker threads.

## Roadmap / TODO

### Phase 0 — Project foundation
- [x] Define package name, public API, and supported Node.js versions.
- [x] Set up TypeScript source and native C/C++ build structure.
- [x] Set up N-API bindings.
- [x] Add development, build, test, and benchmark scripts.
- [x] Add CI for supported platforms.
- [x] Document native build prerequisites.
- [x] Pin and document the miniaudio version used by the project.

### Phase 1 — PCM processing MVP
- [x] Define `AudioFormat` (`s16`, `f32`).
- [x] Define sample rate and channel configuration.
- [x] Implement `createDSP()`.
- [x] Implement block-based `process()`.
- [x] Implement `reset()` and `destroy()`.
- [x] Validate input buffer alignment and frame counts.
- [x] Guarantee no per-sample JS/native calls.
- [x] Add silence, sine-wave, impulse, and random-PCM tests.

### Phase 2 — Native miniaudio integration
- [x] Integrate miniaudio as a vendored/native dependency.
- [x] Use miniaudio for PCM/DSP functionality only.
- [x] Avoid `ma_device` and hardware audio I/O.
- [x] Define a small native DSP context around miniaudio.
- [x] Ensure native resources are released on `destroy()`.
- [x] Make repeated `reset()` safe.
- [x] Add native error handling and status propagation.

### Phase 3 — Basic filters
- [x] Volume / gain.
- [x] Mute.
- [x] Pan/balance for stereo processing.
- [x] Basic clipping protection.
- [x] Parameter validation and safe ranges.
- [x] Runtime parameter updates without rebuilding the DSP instance.

### Phase 4 — Biquad filters
- [x] Implement biquad filter abstraction.
- [x] Low-pass / high-pass / band-pass / notch.
- [x] Peaking EQ / low shelf / high shelf.
- [x] Stable coefficient recalculation.
- [x] Per-channel state handling.
- [x] Tests for frequency response and stability.

### Phase 5 — EQ
- [x] Define backend-neutral EQ band types.
- [x] Implement multi-band EQ using biquads.
- [x] Add low-shelf and high-shelf support.
- [x] Add runtime band parameter updates.
- [x] Preserve filter state for compatible parameter changes.
- [x] Add frequency-response tests.

### Phase 6 — Filter graph
- [x] Introduce a native filter graph abstraction.
- [x] Add/remove filters without restarting the processing pipeline.
- [x] Define stable filter IDs.
- [x] Support filter ordering.
- [x] Support parameter updates in-place.
- [x] Define graph reset semantics.
- [x] Define single-owner/thread-safety rules.

### Phase 7 — Dynamics processing
- [x] Limiter.
- [x] Compressor.
- [x] Soft clipping.
- [x] Attack/release behavior tests.

API:

```ts
const dsp = createDSP({ sampleRate: 48000, channels: 2, format: "f32" });
dsp.setCompressor({ threshold: -18, ratio: 4, attack: 10, release: 100 });
dsp.setLimiter({ threshold: -1, release: 50 });
dsp.setSoftClip(true, 2);
const output = dsp.process(pcm);
```

Dynamics use block processing with per-channel envelope state. Compressor threshold is specified in dBFS, ratio is `>= 1`, attack/release are milliseconds, and limiter threshold is `-24..0 dBFS` with a positive release time. Soft clipping uses a bounded `tanh` transfer and configurable drive. `reset()` restores transparent dynamics defaults and clears envelope state.

## Notes

- No audio device I/O is performed by the native layer.
- Decoding and Opus encoding remain outside this package.
- The synchronous API establishes a single-owner rule; callers are responsible for avoiding concurrent access to one DSP instance.

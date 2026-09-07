# @ziji/audio-dsp

Native DSP engine for ZiPlayer.

`@ziji/audio-dsp` is intended to provide a low-overhead native PCM processing layer for ZiPlayer. The engine focuses on real-time DSP rather than decoding media, playing to hardware devices, or replacing FFmpeg as a general-purpose media tool.

## Goals

- Native, low-overhead PCM processing for Node.js.
- Use miniaudio as the native DSP foundation where appropriate.
- Process audio in blocks instead of per-sample JavaScript calls.
- Support 48 kHz stereo as a first-class ZiPlayer configuration.
- Allow filter parameters to change without restarting an external FFmpeg process.
- Provide deterministic lifecycle and cleanup behavior.
- Keep the DSP API independent from ZiPlayer internals.
- Leave decoding and Opus encoding to dedicated components.

## Non-goals

- General-purpose media decoding.
- Replacing FFmpeg for arbitrary media conversion.
- Audio device input/output.
- Calling native code once per sample.
- Coupling the package to Discord or `@discordjs/voice`.

## Architecture

```text
Source / Decoder
       |
       v
      PCM
       |
       v
+-------------------+
| @ziji/audio-dsp   |
|                   |
| miniaudio / DSP   |
| Volume            |
| Biquad            |
| EQ                |
| Limiter           |
| Custom filters    |
+-------------------+
       |
       v
      PCM
       |
       v
  Opus Encoder
       |
       v
 @discordjs/voice
```

The package should operate on raw PCM. Decoder and encoder responsibilities remain outside this package.

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
- [x] Implement `reset()`.
- [x] Implement `destroy()`.
- [x] Validate input buffer alignment and frame counts.
- [x] Guarantee no per-sample JS/native calls.
- [x] Add silence, sine-wave, impulse, and random-PCM tests.
- [x] Verify repeated processing does not leak resources.

Target first configuration:

- Sample rate: 48,000 Hz
- Channels: 2
- Block size: 480 or 960 frames
- Format: `s16` and/or `f32`

### Phase 2 — Native miniaudio integration

- [x] Integrate miniaudio as a vendored/native dependency.
- [x] Use miniaudio for PCM/DSP functionality only.
- [x] Avoid `ma_device` and hardware audio I/O.
- [x] Define a small native DSP context around miniaudio.
- [x] Ensure native resources are released on `destroy()`.
- [x] Make repeated `reset()` safe.
- [x] Add native error handling and status propagation.

### Phase 3 — Basic filters

- [ ] Volume / gain.
- [ ] Mute.
- [ ] Pan/balance if required.
- [ ] Basic clipping protection.
- [ ] Parameter validation and safe ranges.
- [ ] Runtime parameter updates without rebuilding the DSP instance.

Example API direction:

```ts
const dsp = createDSP({
  sampleRate: 48000,
  channels: 2,
  format: "s16",
});

dsp.setVolume(0.8);
const output = dsp.process(pcm);
```

### Phase 4 — Biquad filters

- [ ] Implement biquad filter abstraction.
- [ ] Low-pass.
- [ ] High-pass.
- [ ] Band-pass.
- [ ] Notch.
- [ ] Peaking EQ.
- [ ] Low shelf.
- [ ] High shelf.
- [ ] Stable coefficient recalculation.
- [ ] Per-channel state handling.
- [ ] Tests for frequency response and stability.

### Phase 5 — EQ

- [ ] Define backend-neutral EQ band types.
- [ ] Implement multi-band EQ using biquads.
- [ ] Add low-shelf and high-shelf support.
- [ ] Add runtime band parameter updates.
- [ ] Ensure filter state is preserved when only parameters change.
- [ ] Add frequency-response tests.

Example direction:

```ts
dsp.setEQ([
  { type: "lowShelf", frequency: 100, gain: 5 },
  { type: "peaking", frequency: 1000, gain: 3, q: 1 },
  { type: "highShelf", frequency: 8000, gain: 2 },
]);
```

### Phase 6 — Filter graph

- [ ] Introduce a native filter graph abstraction.
- [ ] Add/remove filters without restarting the processing pipeline.
- [ ] Define stable filter IDs.
- [ ] Support filter ordering.
- [ ] Support parameter updates in-place.
- [ ] Define graph reset semantics.
- [ ] Ensure graph mutation cannot race with processing.
- [ ] Define single-owner/thread-safety rules.

Expected processing model:

```text
PCM
 |
 v
Volume -> EQ -> Limiter -> PCM
```

### Phase 7 — Dynamics processing

- [ ] Limiter.
- [ ] Compressor if required.
- [ ] Soft clipping if required.
- [ ] Attack/release behavior tests.

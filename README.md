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
| Mute              |
| Pan / Balance     |
| Clipping          |
| Biquad            |
| EQ                |
| Filter Graph      |
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
- [x] Low-pass.
- [x] High-pass.
- [x] Band-pass.
- [x] Notch.
- [x] Peaking EQ.
- [x] Low shelf.
- [x] High shelf.
- [x] Stable coefficient recalculation.
- [x] Per-channel state handling.
- [x] Tests for frequency response and stability.

### Phase 5 — EQ
- [x] Define backend-neutral EQ band types.
- [x] Implement multi-band EQ using biquads.
- [x] Add low-shelf and high-shelf support.
- [x] Add runtime band parameter updates.
- [x] Ensure filter state is preserved when only parameters change.
- [x] Add frequency-response tests.

### Phase 6 — Filter graph
- [x] Introduce a native filter graph abstraction.
- [x] Add/remove filters without restarting the processing pipeline.
- [x] Define stable filter IDs.
- [x] Support filter ordering.
- [x] Support parameter updates in-place.
- [x] Define graph reset semantics.
- [x] Ensure graph mutation cannot race with processing.
- [x] Define single-owner/thread-safety rules.

API:

```ts
const dsp = createDSP({ sampleRate: 48000, channels: 2, format: "f32" });
dsp.addEQ("eq", [
  { type: "lowShelf", frequency: 100, gain: 4 },
  { type: "peaking", frequency: 1000, q: 1, gain: 3 },
  { type: "highShelf", frequency: 8000, gain: 2 },
]);
dsp.addBiquad("tone", { type: "lowPass", frequency: 12000, q: 0.707 });
dsp.setFilterOrder(["eq", "tone"]);
const output = dsp.process(pcm);
dsp.updateBiquad("tone", { type: "lowPass", frequency: 10000, q: 0.707 });
dsp.removeFilter("tone");
```

The graph is owned by the DSP instance and is synchronously mutated through the JS API; `process()` is synchronous, so graph mutation cannot interleave with native processing. Filter IDs are unique within an instance, ordering is explicit, and in-place updates retain compatible biquad delay-line state. `reset()` clears graph configuration and processing state.

### Phase 7 — Dynamics processing
- [ ] Limiter.
- [ ] Compressor if required.
- [ ] Soft clipping if required.
- [ ] Attack/release behavior tests.

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

- [ ] Define package name, public API, and supported Node.js versions.
- [ ] Set up TypeScript source and native C/C++ build structure.
- [ ] Set up N-API bindings.
- [ ] Add development, build, test, and benchmark scripts.
- [ ] Add CI for supported platforms.
- [ ] Document native build prerequisites.
- [ ] Pin and document the miniaudio version used by the project.

### Phase 1 — PCM processing MVP

- [ ] Define `AudioFormat` (`s16`, `f32`).
- [ ] Define sample rate and channel configuration.
- [ ] Implement `createDSP()`.
- [ ] Implement block-based `process()`.
- [ ] Implement `reset()`.
- [ ] Implement `destroy()`.
- [ ] Validate input buffer alignment and frame counts.
- [ ] Guarantee no per-sample JS/native calls.
- [ ] Add silence, sine-wave, impulse, and random-PCM tests.
- [ ] Verify repeated processing does not leak resources.

Target first configuration:

- Sample rate: 48,000 Hz
- Channels: 2
- Block size: 480 or 960 frames
- Format: `s16` and/or `f32`

### Phase 2 — Native miniaudio integration

- [ ] Integrate miniaudio as a vendored/native dependency.
- [ ] Use miniaudio for PCM/DSP functionality only.
- [ ] Avoid `ma_device` and hardware audio I/O.
- [ ] Define a small native DSP context around miniaudio.
- [ ] Ensure native resources are released on `destroy()`.
- [ ] Make repeated `reset()` safe.
- [ ] Add native error handling and status propagation.

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
- [ ] Verify no NaN/Infinity output.
- [ ] Verify predictable clipping behavior.

### Phase 8 — Resampling

- [ ] Add resampling as a separate pipeline operation.
- [ ] Support common input sample rates.
- [ ] Preserve channel layout.
- [ ] Test frame-count correctness.
- [ ] Benchmark quality/performance trade-offs.
- [ ] Keep resampling separate from the normal filter graph.

Recommended pipeline:

```text
Decoder -> Resampler -> DSP -> Opus Encoder
```

### Phase 9 — Memory and performance

- [ ] Reuse native buffers where possible.
- [ ] Avoid unnecessary JS/native copies.
- [ ] Benchmark `s16` vs `f32`.
- [ ] Benchmark 240/480/960 frame blocks.
- [ ] Investigate SIMD where useful.
- [ ] Measure allocations and garbage-collection pressure.
- [ ] Measure native call overhead.
- [ ] Ensure processing stays comfortably below real-time budget.

Important metric:

```text
processing_time / audio_duration
```

For example, a 20 ms block should be processed substantially faster than 20 ms on the target hardware.

### Phase 10 — Lifecycle and cancellation

- [ ] Make `destroy()` idempotent.
- [ ] Release all native resources on destroy.
- [ ] Make processing after destroy fail predictably.
- [ ] Add optional `AbortSignal` integration at the lifecycle level.
- [ ] Ensure reset/destroy cannot leave native state alive.
- [ ] Test repeated create/process/reset/destroy cycles.

`AbortSignal` should control the DSP instance lifecycle, not be passed through every audio sample/block operation.

### Phase 11 — Audio correctness

- [ ] Test silence preservation.
- [ ] Test impulse response.
- [ ] Test sine waves at multiple frequencies.
- [ ] Test stereo channel independence.
- [ ] Test frame-count preservation.
- [ ] Test `s16` bounds.
- [ ] Test `f32` finite-value guarantees.
- [ ] Detect/prevent NaN and Infinity propagation.
- [ ] Test filter reset behavior.
- [ ] Test abrupt parameter changes for audible instability.

### Phase 12 — Benchmarks

- [ ] Add reproducible native DSP benchmarks.
- [ ] Volume benchmark.
- [ ] One-biquad benchmark.
- [ ] 5-band EQ benchmark.
- [ ] 10-band EQ benchmark.
- [ ] 20-filter benchmark.
- [ ] EQ + limiter benchmark.
- [ ] Resampler benchmark.
- [ ] Buffer allocation benchmark.
- [ ] Native call overhead benchmark.
- [ ] Record CPU time, wall time, allocations, and real-time ratio.

Do not publish player-count or CPU claims without reproducible benchmarks on a documented test environment.

### Phase 13 — ZiPlayer integration

- [ ] Define a backend-neutral `AudioFilter` model.
- [ ] Implement `NativeDSPBackend`.
- [ ] Adapt `FilterController` to use the native backend.
- [ ] Keep FFmpeg backend as a compatibility/fallback path where necessary.
- [ ] Remove filter-change-driven FFmpeg process restarts for supported native filters.
- [ ] Reset DSP state correctly after seek/recreate.
- [ ] Integrate with `AbortSignal` and player lifecycle.
- [ ] Verify no stale DSP instance survives track transitions.
- [ ] Verify seek/recovery generation handling remains race-safe.

Desired behavior:

```text
applyFilter()
    |
    v
update native DSP state
    |
    v
next PCM block uses new parameters
```

No FFmpeg process restart should be required for a supported native filter parameter change.

### Phase 14 — Packaging and release

- [ ] Define supported OS/CPU targets.
- [ ] Build native binaries for supported Node.js versions.
- [ ] Decide between source builds and prebuilt binaries.
- [ ] Add package exports.
- [ ] Add TypeScript declarations.
- [ ] Add release automation.
- [ ] Add changelog.
- [ ] Document ABI/runtime compatibility.
- [ ] Verify clean installation from npm.
- [ ] Publish first alpha release.

## Suggested public API

Keep the initial API small:

```ts
export interface DSPOptions {
  sampleRate: number;
  channels: number;
  format: "s16" | "f32";
}

export interface AudioDSP {
  process(input: Buffer, output?: Buffer): Buffer;
  reset(): void;
  destroy(): void;

  setVolume(gain: number): void;
  setEQ(bands: EQBand[]): void;
}

export function createDSP(options: DSPOptions): AudioDSP;
```

The API should evolve around audio concepts rather than FFmpeg filter strings.

## Filter model

Avoid making the public model depend on FFmpeg syntax.

Prefer:

```ts
interface AudioFilter {
  id: string;
  type: string;
  params: Record<string, number | string | boolean>;
}
```

rather than:

```ts
interface AudioFilter {
  ffmpegFilter: string;
}
```

This allows multiple backends:

```text
AudioFilter
   |
   +-- NativeDSPBackend
   |
   +-- FFmpegBackend
   |
   +-- FutureBackend
```

## Performance principles

1. Process PCM in blocks.
2. Avoid per-sample JavaScript calls.
3. Minimize JS/native boundary crossings.
4. Reuse buffers where practical.
5. Keep filter state in native memory.
6. Update parameters in-place when possible.
7. Do not spawn external processes for ordinary filter changes.
8. Benchmark before adding complicated optimizations.

## Testing strategy

The test suite should cover three levels:

### Unit tests

Validate API behavior, parameter validation, lifecycle, and deterministic state transitions.

### DSP tests

Validate signal behavior using generated PCM fixtures such as sine waves, impulses, silence, and noise.

### Integration tests

Run the complete path:

```text
PCM -> audio-dsp -> Opus -> ZiPlayer audio pipeline
```

and verify playback continuity, seek behavior, track transitions, and cleanup.

## ZiPlayer integration target

The long-term goal is:

```text
                 ZiPlayer
                    |
             StreamController
                    |
                 Decoder
                    |
                   PCM
                    |
            @ziji/audio-dsp
                    |
             Native DSP Graph
                    |
                   PCM
                    |
              Opus Encoder
                    |
             @discordjs/voice
```

`@ziji/audio-dsp` should remain reusable outside ZiPlayer even though ZiPlayer is its primary consumer.

## Status

🚧 Early development — API and native architecture are subject to change.

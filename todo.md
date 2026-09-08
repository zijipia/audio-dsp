# audio-dsp implementation TODO

> Goal: replace FFmpeg-based DSP work in ZiPlayer with an in-process native DSP pipeline while keeping source timeline/seek ownership in the decoder/miniaudio `ma_data_source` layer.

## Current state

- [x] Phase 0 foundation: TypeScript + native/N-API structure, CI, build scripts, miniaudio v0.11.25 pinned.
- [x] Core PCM processing is present on `main`.
- [x] Biquad/EQ/filter graph APIs are present on `main`.
- [x] Compressor/limiter/soft-clip processing is present on `main`.
- [ ] Seek-aware DSP state reset.
- [ ] miniaudio `ma_data_source` DSP adapter.
- [ ] ZiPlayer `FilterController` migration away from FFmpeg.
- [ ] Native-vs-FFmpeg seek/filter-change benchmark.
- [ ] Optional DSP preroll after basic seek is proven.

## Phase 3 — DSP state semantics

- [ ] Add native `resetProcessingState()` that clears temporal state only.
- [ ] Preserve filter configuration, coefficients, volume/pan/mute and dynamics parameters.
- [ ] Expose JS `resetState()`.
- [ ] Keep existing `reset()` as the full configuration reset.
- [ ] Add tests for biquad/EQ state reset.
- [ ] Add tests for compressor/limiter envelope reset.
- [ ] Add tests proving configuration survives `resetState()`.
- [ ] Add tests proving normal block processing preserves state.

## Phase 4 — miniaudio DSP data source

- [ ] Add `native/dsp_data_source.h`.
- [ ] Add `native/dsp_data_source.cc`.
- [ ] Implement `read()` over an upstream `ma_data_source` followed by DSP processing.
- [ ] Implement `seek()` using `ma_data_source_seek_to_pcm_frame()` on the upstream source.
- [ ] Reset DSP temporal state only after a successful upstream seek.
- [ ] Implement `get_data_format()`.
- [ ] Implement `get_cursor()`.
- [ ] Implement `get_length()`.
- [ ] Define ownership/lifetime rules for upstream source and DSP context.
- [ ] Add cursor/EOF/seek failure tests.
- [ ] Add concurrent read/seek safety rules/tests where applicable.
- [ ] Add the new native source to `binding.gyp` only when its API is ready.

## Phase 5 — ZiPlayer integration

- [ ] Map structured filter configuration to native DSP setters.
- [ ] Remove FFmpeg filter-string generation from the native DSP path.
- [ ] Reuse one DSP instance across runtime filter changes.
- [ ] Route seek to the decoder/source first, then reset DSP temporal state.
- [ ] Ensure filter change + seek preserves configuration and resets temporal state once.
- [ ] Keep Opus encoding separate from the DSP layer.
- [ ] Remove FFmpeg process recreation from filter/seek operations.
- [ ] Add integration tests for filter changes and repeated seeks.

## Phase 6 — performance

- [ ] Benchmark filter change: FFmpeg restart/rebuild vs native setter update.
- [ ] Benchmark seek-to-first-output latency.
- [ ] Benchmark steady-state CPU/throughput.
- [ ] Measure allocations and JS/native boundary overhead.
- [ ] Document codec/source-dependent decoder seek cost separately from DSP seek cost.

## Phase 7 — production hardening

- [ ] Add edge-case seek tests: zero, EOF, forward, backward, repeated seek.
- [ ] Verify failed seeks do not mutate DSP state.
- [ ] Verify destroyed DSP cannot be reused.
- [ ] Verify 1–8 channel behavior where supported.
- [ ] Add API/migration documentation.
- [ ] Add optional preroll only after benchmark and audio-quality validation.
- [ ] Release checklist and versioning.

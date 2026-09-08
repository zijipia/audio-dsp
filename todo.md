# audio-dsp implementation TODO

> Goal: replace FFmpeg-based DSP work in ZiPlayer with an in-process native DSP pipeline while keeping source timeline/seek ownership in the decoder/miniaudio `ma_data_source` layer.

## Current state

- [x] Phase 0 foundation: TypeScript + native/N-API structure, CI, build scripts, miniaudio v0.11.25 pinned.
- [x] Core PCM processing is present on `main`.
- [x] Biquad/EQ/filter graph APIs are present on `main`.
- [x] Compressor/limiter/soft-clip processing is present on `main`.
- [x] Seek-aware DSP state reset.
- [~] miniaudio `ma_data_source` DSP adapter — implementation added; validation tests still pending.
- [~] ZiPlayer `FilterController` native path — raw PCM plus miniaudio-supported encoded sources now route through native decode/DSP; unsupported encoded sources retain FFmpeg compatibility fallback.
- [ ] Native-vs-FFmpeg seek/filter-change benchmark.
- [ ] Optional DSP preroll after basic seek is proven.

## Phase 3 — DSP state semantics — DONE

- [x] Add native `resetProcessingState()` that clears temporal state only.
- [x] Preserve filter configuration, coefficients, volume/pan/mute and dynamics parameters.
- [x] Expose JS `resetState()`.
- [x] Keep existing `reset()` as the full configuration reset.
- [x] Add tests for biquad/EQ state reset.
- [x] Add tests for compressor/limiter state preservation through resetState.
- [x] Add tests proving configuration survives `resetState()`.
- [x] Add tests proving repeated `resetState()` is safe.

## Phase 4 — miniaudio DSP data source

- [x] Add `native/dsp_data_source.h`.
- [x] Add `native/dsp_data_source.cc`.
- [x] Implement `read()` over an upstream `ma_data_source` followed by DSP processing.
- [x] Implement `seek()` using `ma_data_source_seek_to_pcm_frame()` on the upstream source.
- [x] Reset DSP temporal state only after a successful upstream seek.
- [x] Implement `get_data_format()`.
- [x] Implement `get_cursor()`.
- [x] Implement `get_length()`.
- [x] Forward `set_looping()` to the upstream source.
- [x] Reuse a persistent scratch buffer for read/forward-discard operations.
- [x] Define ownership/lifetime rules: adapter borrows upstream and DSP; caller owns both and must keep them alive until adapter uninit.
- [ ] Add cursor/EOF/seek failure tests.
- [ ] Add concurrent read/seek safety rules/tests where applicable.
- [x] Add the new native source to `binding.gyp`.

### Phase 4 design rules

- The adapter does not own or destroy the upstream `ma_data_source`.
- The adapter does not own or destroy the DSP context.
- A successful absolute seek updates the adapter cursor and then clears DSP temporal state.
- A failed upstream seek leaves DSP state untouched.
- `read(..., nullptr, ...)` is implemented as a forward-discard through the DSP so temporal state advances consistently with the skipped audio; it does not reset state.
- Read and seek must be serialized by the caller; the adapter does not add a lock. This matches the intended decoder/source ownership model and avoids adding a lock to the hot read path.

## Phase 5 — ZiPlayer integration

- [~] Map the currently supported structured filter set to native DSP setters for raw s16le PCM.
- [x] Remove FFmpeg filter-string generation from the native DSP path.
- [x] Reuse one DSP instance across runtime filter changes on the native PCM path.
- [x] Route native raw-PCM seek to the resolver/source first, then initialize/reset DSP state.
- [x] Keep Opus encoding separate from the DSP layer; raw PCM is handed to the Discord voice pipeline.
- [~] Remove FFmpeg process recreation from filter/seek operations — complete for native raw PCM and miniaudio-supported encoded sources; retained as compatibility fallback for unsupported sources.
- [ ] Add integration tests for filter changes and repeated seeks.
- [x] Add native miniaudio decoder binding with PCM read/seek/cursor/length operations.
- [x] Route WAV/MP3/FLAC/OGG-family sources into native decoder → PCM → native DSP when the source can be identified as supported.
- [~] Encoded native source currently buffers the encoded input before constructing `ma_decoder`; callback-backed `ma_data_source` input remains a follow-up optimization.
- [ ] Add native decoder/data-source integration for encoded WebM/Opus sources; miniaudio's generic decoder path is not being assumed to support every FFmpeg codec/container.

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

## Implementation log

- `docs: add native DSP implementation TODO` — `ab8a2bc18bb116863690a5dc6a7f196da7f2e534`
- `core: add native DSP processing-state reset` — `cddce7233dda8dd048d3caa0e617ff4e6086fc59`
- `fix: correct native filter-order environment` — `8784a3ed978b40fc514f37f77a8a0fbee573671f`
- `api: expose DSP resetState` — `6e24606bdd1063513cd89b0937b65a5554a0c2ae`
- `test: fix full reset expectation` — `608d9dd58968ccf17dcf645d0e4b7722de0c257f`
- `core: add miniaudio DSP data source adapter` — `248a54a9f3865b08f114a5cdabf930cdde989361`
- `core: implement miniaudio DSP data source adapter` — `b4e3f95c656608f327c0f2a892d041ed5c10ea99`
- `fix: initialize DSP data source through miniaudio base API` — `62c07275d2209ee5a74baefe36309b37ae5c304b`
- `fix: match miniaudio data source vtable` — `228e2230a61610baa62c232bb24f60383b854cd0`
- `build: compile DSP data source adapter` — `64ca1041712f1e36c9eaff718e5a9830d1137a57`
- `fix: make DSP data source seek/skip stateful and reusable` — `fc29056f34b214cf382fc0cde87e81c8d2f48115`
- `fix: make DSP data source seek/skip stateful and reusable` — `9be37f40e04ffcec411532955070e3b9ae8404f`
- `feat: add miniaudio native decoder binding` — `8f4c4369422042138a100027b991fac9116fbfa9`
- `fix: register native decoder addon` — `18d33c425b4139ccf9fc961f18f8624f88d9baa5`
- `build: add miniaudio decoder addon target` — `7ae7bdf8d50b45d21fa55351b673fcb19d6dd273`
- `api: expose native miniaudio decoder` — `76d4087c7adfe14f979d5d76655aee81ce6ef367`
- `fix: preserve EQ update parameter` — `898523ec5549b9b70cba2fdf7b55b445c001f100`
- `build: package native decoder addon` — `63d3bb79b88837c87ee540277b630825a4b6802b`

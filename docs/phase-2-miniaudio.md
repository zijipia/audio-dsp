# Phase 2 — Native miniaudio integration

Status: **implemented on `main`**

## Completed

- [x] Pin miniaudio v0.11.25 at commit `9634bedb5b5a2ca38c1ee7108a9358a4e233f14d`.
- [x] Track miniaudio as a pinned native submodule under `third_party/miniaudio`.
- [x] Compile a dedicated miniaudio implementation unit with `MA_NO_DEVICE_IO` and `MA_NO_THREADING`.
- [x] Use miniaudio's `ma_data_converter` for PCM block processing.
- [x] Keep the package independent from `ma_device` and hardware audio I/O.
- [x] Store the converter and stream configuration inside the native DSP context.
- [x] Release converter resources on explicit `destroy()` and finalization.
- [x] Reinitialize the converter on repeated `reset()` calls.
- [x] Propagate miniaudio initialization and processing failures as native exceptions.
- [x] Add Phase 2 lifecycle and PCM correctness coverage.

## Processing model

Phase 2 keeps the Phase 1 processing contract intentionally transparent: `s16` remains `s16`, `f32` remains `f32`, the sample rate and channel count remain unchanged, and a whole PCM block is passed to miniaudio in one native call.

No `ma_device` is initialized. Audio hardware, playback, capture, decoding, and encoding remain outside this package.

## Dependency model

The repository uses a Git submodule because miniaudio is distributed as a single-header implementation and is maintained as an external upstream project. The submodule is pinned to the exact v0.11.25 release commit so builds are reproducible.

CI checks out the repository with submodules enabled before running the native build and test suite.

## Version

Package version: `0.3.0`

Native addon version: `0.3.0-phase2-miniaudio`

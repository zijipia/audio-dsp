# Phase 1 — PCM processing MVP

Status: **implemented on `main`**

## Completed

- [x] `AudioFormat`: `s16` and `f32`.
- [x] Sample-rate and channel configuration validation.
- [x] `createDSP()` backed by a native N-API context.
- [x] Block-based `process(input)` with one JS → native call per PCM block.
- [x] `reset()` lifecycle operation.
- [x] Idempotent `destroy()` and post-destroy guards.
- [x] PCM frame-alignment validation.
- [x] Silence, sine-wave, impulse, random-PCM, empty-block, and lifecycle tests.
- [x] Repeated create/process/reset/destroy coverage.

## Processing contract

Phase 1 is intentionally a transparent PCM pipeline: input bytes are copied to a fresh output buffer without gain or filtering. This establishes the processing, ownership, validation, and lifecycle contract before Phase 2 native miniaudio integration and later filter phases.

The first-class configuration is 48 kHz stereo with 480/960-frame blocks and `s16`/`f32` PCM.

## Validation note

The repository CI workflow is configured to build and run the test suite on Ubuntu, macOS, and Windows with Node.js 22 and 24. This change was pushed directly to `main`; CI status should be treated as the authoritative build validation.

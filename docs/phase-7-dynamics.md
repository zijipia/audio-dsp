# Phase 7 — Dynamics processing

Status: implemented.

## Processing chain

The native processor applies volume/pan and the configured filter graph, then dynamics processing:

```text
PCM -> filters -> compressor -> limiter -> optional soft clip -> PCM
```

The compressor and limiter therefore inspect the post-filter signal before the optional soft-clip transfer is applied. The final clamp is the output safety boundary; there is no intermediate clamp between volume/pan and the filter graph.

## Limiter

`setLimiter({ threshold, release })` enables a peak limiter. Threshold is in dBFS and accepts `-24..0`; release is in milliseconds and must be positive. The limiter maintains a per-channel gain envelope across blocks.

## Compressor

`setCompressor({ threshold, ratio, attack, release })` enables downward compression. Threshold is `-60..0 dBFS`, ratio is `>= 1`, and attack/release are positive milliseconds. Envelope state is preserved across blocks.

## Soft clipping

`setSoftClip(true, drive)` enables a bounded `tanh` transfer. Drive is positive and capped at 20. Disabling it restores the unprocessed transfer for this stage. Soft clipping is deliberately the final nonlinear stage after the limiter.

## Runtime and reset

All parameters can be changed without recreating the DSP context. `reset()` clears the filter graph and dynamics envelopes and restores transparent defaults. The API is synchronous and establishes a single-owner rule: one DSP instance must not be mutated or processed concurrently from multiple threads.

## Validation and tests

The TypeScript API validates public parameter ranges before entering native code. Native code validates again at the boundary, including volume (`0..4`), pan (`-1..1`), and mute type. Tests cover limiter peak control, compressor gain reduction, soft-clip bounds, runtime updates, documented dynamics ordering, volume-before-filter behavior, native control validation, invalid parameters, and coexistence with the Phase 6 filter graph.

Native version: `0.8.0-phase7-dynamics`.

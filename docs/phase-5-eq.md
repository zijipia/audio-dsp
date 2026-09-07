# Phase 5 — EQ

Status: **implemented on `main`**

## Completed

- Backend-neutral `EQBand` API with `lowShelf`, `peaking`, and `highShelf` band types.
- Multi-band EQ implemented as a native chain of Phase 4 biquads.
- Up to 16 bands per DSP instance.
- Runtime `setEQ()` and `clearEQ()` APIs.
- Per-channel filter state for every EQ band.
- EQ validation for type, frequency, Q, gain, and maximum band count.
- Frequency must be below Nyquist; Q is positive; gain is limited to -24..24 dB.
- `reset()` clears EQ configuration and state.
- EQ runs after Phase 3 volume/mute/pan and the optional single Phase 4 biquad.
- No audio device I/O is introduced.

## API

```ts
import { createDSP } from "@ziji/audio-dsp";

const dsp = createDSP({ sampleRate: 48000, channels: 2, format: "f32" });

dsp.setEQ([
  { type: "lowShelf", frequency: 100, gain: 5 },
  { type: "peaking", frequency: 1000, gain: 3, q: 1 },
  { type: "highShelf", frequency: 8000, gain: 2 },
]);

const output = dsp.process(pcm);
dsp.clearEQ();
```

`q` defaults to `1 / sqrt(2)`. Gain is required for EQ bands and is interpreted in dB.

## Runtime semantics

`setEQ()` validates and builds the complete replacement band chain before swapping it into the native context, so invalid configurations do not partially mutate the existing chain. A newly supplied chain starts with zeroed delay-line state. The Phase 6 filter graph will provide stable IDs and true in-place parameter updates while preserving state for individual bands.

## Version

Package version: `0.6.0`

Native addon version: `0.6.0-phase5-eq`

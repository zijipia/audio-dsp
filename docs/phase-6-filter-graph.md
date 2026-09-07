# Phase 6 — Filter graph

Status: **implemented on `main`**

## Completed

- Native filter graph owned by each DSP instance.
- Stable, unique filter IDs.
- Biquad and multi-band EQ graph nodes.
- Add/remove filters without rebuilding the DSP context.
- Explicit filter ordering.
- In-place biquad/EQ parameter updates.
- Compatible biquad updates preserve per-channel delay-line state.
- `reset()` clears graph configuration and processing state.
- Synchronous `process()` plus synchronous graph mutation means graph changes cannot interleave with processing in this API; the DSP instance is single-owner and is not a cross-thread processing primitive.
- No audio device I/O.

## API

```ts
const dsp = createDSP({ sampleRate: 48000, channels: 2, format: "f32" });

dsp.addEQ("eq", [
  { type: "lowShelf", frequency: 100, gain: 4 },
  { type: "peaking", frequency: 1000, q: 1, gain: 3 },
  { type: "highShelf", frequency: 8000, gain: 2 },
]);
dsp.addBiquad("tone", { type: "lowPass", frequency: 12000, q: 0.707 });
dsp.setFilterOrder(["eq", "tone"]);

dsp.updateBiquad("tone", { type: "lowPass", frequency: 10000, q: 0.707 });
dsp.removeFilter("tone");
```

Graph processing runs in the configured order after volume/mute/pan. Legacy `setBiquad()` and `setEQ()` APIs remain available when no graph nodes are configured.

## Version

Package version: `0.7.0`

Native addon version: `0.7.0-phase6-filter-graph`

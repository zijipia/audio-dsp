# @ziji/audio-dsp

Native, low-overhead DSP engine for Node.js and ZiPlayer.

`@ziji/audio-dsp` is a native Node.js audio DSP library for processing PCM audio in blocks. It provides common playback-oriented processing primitives without handling audio decoding, encoding, device I/O, or media playback.

The library is designed to keep the hot audio path in native code while exposing a small synchronous TypeScript API.

## Features

- Native N-API implementation
- Node.js `>= 22`
- PCM `s16` and `f32`
- Mono and multi-channel processing
- Configurable sample rate
- Volume / gain
- Mute
- Stereo pan / balance
- Biquad filters
- Multi-band EQ
- Native filter graph
- Runtime filter updates
- Compressor
- Limiter
- Soft clipping
- Per-channel filter state
- Per-channel dynamics envelope state
- Block-based processing
- Explicit DSP lifecycle
- Cross-platform native build
- Automated GitHub Release builds

## Processing pipeline

The current processing pipeline is:

```text
                    ┌─────────────────┐
PCM Input ─────────►│ Volume / Mute   │
                    │ Pan / Balance   │
                    └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │   Filter Graph  │
                    │                 │
                    │  Biquad / EQ    │
                    └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │   Compressor    │
                    └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │     Limiter     │
                    └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │    Soft Clip    │
                    └────────┬────────┘
                             │
                             ▼
                        PCM Output
```

All processing is performed block-by-block. The native layer does not perform audio device I/O.

## Installation

Install the package from npm:

```bash
npm install @ziji/audio-dsp
```

For development from source:

```bash
git clone https://github.com/zijipia/audio-dsp.git
cd audio-dsp

git submodule update --init --recursive

npm install
npm test
```

## Quick start

```ts
import { createDSP } from "@ziji/audio-dsp";

const dsp = createDSP({
  sampleRate: 48000,
  channels: 2,
  format: "f32",
});

const output = dsp.process(input);

dsp.destroy();
```

`process()` accepts a Node.js `Buffer` containing interleaved PCM frames and returns a new `Buffer`.

The input buffer must contain complete audio frames.

## Audio formats

Supported formats:

```ts
type AudioFormat = "s16" | "f32";
```

### `s16`

Signed 16-bit PCM.

Samples are interpreted in the normal signed 16-bit PCM range.

### `f32`

32-bit floating-point PCM.

Samples are processed using normalized floating-point audio values.

## DSP configuration

```ts
const dsp = createDSP({
  sampleRate: 48000,
  channels: 2,
  format: "f32",
});
```

Configuration:

| Option | Description |
| --- | --- |
| `sampleRate` | Sample rate in Hz |
| `channels` | Number of audio channels |
| `format` | `s16` or `f32` |

The sample rate and channel configuration belong to the DSP instance and remain fixed for its lifetime.

## Volume

```ts
dsp.setVolume(1.0);
```

Volume is a linear gain multiplier.

Examples:

```ts
dsp.setVolume(0.5); // approximately -6 dB
dsp.setVolume(1.0); // unity gain
dsp.setVolume(2.0); // approximately +6 dB
```

The implementation includes output clipping protection.

## Mute

```ts
dsp.setMute(true);
dsp.setMute(false);
```

Mute is applied before the filter and dynamics stages.

## Stereo pan

```ts
dsp.setPan(-1);
dsp.setPan(0);
dsp.setPan(1);
```

For stereo audio:

```text
-1 = left
 0 = center
+1 = right
```

Pan is intended for stereo processing.

## Biquad filters

A single biquad can be configured using:

```ts
dsp.setBiquad({
  type: "lowPass",
  frequency: 12000,
  q: 0.707,
});
```

Supported types:

```ts
type BiquadType =
  | "lowPass"
  | "highPass"
  | "bandPass"
  | "notch"
  | "peaking"
  | "lowShelf"
  | "highShelf";
```

Example:

```ts
dsp.setBiquad({
  type: "peaking",
  frequency: 1000,
  q: 1.0,
  gain: 3,
});
```

Clear the filter:

```ts
dsp.clearBiquad();
```

## Multi-band EQ

The EQ API supports multiple bands:

```ts
dsp.setEQ([
  {
    type: "lowShelf",
    frequency: 100,
    gain: 2,
  },
  {
    type: "peaking",
    frequency: 1000,
    q: 1,
    gain: -2,
  },
  {
    type: "peaking",
    frequency: 5000,
    q: 1.2,
    gain: 3,
  },
  {
    type: "highShelf",
    frequency: 10000,
    gain: 2,
  },
]);
```

Supported EQ band types:

```ts
type EQBandType =
  | "lowShelf"
  | "peaking"
  | "highShelf";
```

The EQ supports up to 16 bands.

Runtime updates preserve compatible filter state where possible.

Clear the EQ:

```ts
dsp.clearEQ();
```

## Filter graph

For applications that need multiple processing stages, use the filter graph.

```ts
dsp.addBiquad("lowpass", {
  type: "lowPass",
  frequency: 12000,
  q: 0.707,
});

dsp.addEQ("tone", [
  {
    type: "peaking",
    frequency: 1000,
    q: 1,
    gain: 2,
  },
]);
```

Filter IDs are stable within the DSP instance.

The graph can be explicitly ordered:

```ts
dsp.setFilterOrder([
  "lowpass",
  "tone",
]);
```

Remove a filter:

```ts
dsp.removeFilter("tone");
```

Update an existing filter without rebuilding the DSP instance:

```ts
dsp.updateBiquad("lowpass", {
  type: "lowPass",
  frequency: 10000,
  q: 0.707,
});
```

Or update an EQ node:

```ts
dsp.updateEQ("tone", [
  {
    type: "peaking",
    frequency: 1500,
    q: 1,
    gain: 3,
  },
]);
```

The filter graph is processed in the order defined by `setFilterOrder()`.

Compatible filter state is preserved during in-place parameter updates.

## Compressor

The compressor provides threshold, ratio, attack and release controls:

```ts
dsp.setCompressor({
  threshold: -18,
  ratio: 4,
  attack: 10,
  release: 100,
});
```

Parameters:

| Parameter | Description |
| --- | --- |
| `threshold` | Threshold in dBFS |
| `ratio` | Compression ratio, `>= 1` |
| `attack` | Attack time in milliseconds |
| `release` | Release time in milliseconds |

Example:

```ts
dsp.setCompressor({
  threshold: -12,
  ratio: 3,
  attack: 5,
  release: 120,
});
```

The compressor maintains envelope state independently for each channel.

## Limiter

```ts
dsp.setLimiter({
  threshold: -1,
  release: 50,
});
```

The limiter threshold is specified in dBFS.

Supported threshold range:

```text
-24 dBFS ... 0 dBFS
```

Release is specified in milliseconds and must be positive.

Example:

```ts
dsp.setLimiter({
  threshold: -0.5,
  release: 80,
});
```

The limiter is intended as the final peak-control stage before output.

## Soft clipping

Soft clipping can be enabled with configurable drive:

```ts
dsp.setSoftClip(true, 2);
```

Disable it:

```ts
dsp.setSoftClip(false);
```

The implementation uses a bounded `tanh` transfer.

Higher drive produces stronger saturation.

## Complete example

```ts
import { createDSP } from "@ziji/audio-dsp";

const dsp = createDSP({
  sampleRate: 48000,
  channels: 2,
  format: "f32",
});

dsp.setVolume(1.0);
dsp.setPan(0);

dsp.addBiquad("lowpass", {
  type: "lowPass",
  frequency: 18000,
  q: 0.707,
});

dsp.addEQ("tone", [
  {
    type: "lowShelf",
    frequency: 100,
    gain: 2,
  },
  {
    type: "peaking",
    frequency: 2500,
    q: 1,
    gain: 1.5,
  },
  {
    type: "highShelf",
    frequency: 10000,
    gain: 1,
  },
]);

dsp.setCompressor({
  threshold: -18,
  ratio: 3,
  attack: 10,
  release: 100,
});

dsp.setLimiter({
  threshold: -1,
  release: 50,
});

dsp.setSoftClip(true, 2);

const output = dsp.process(input);

dsp.destroy();
```

## Lifecycle

A DSP instance has an explicit lifecycle:

```text
createDSP()
    │
    ▼
 process()
    │
    ├── runtime parameter updates
    │
    ├── reset()
    │
    ▼
 destroy()
```

Reset processing state:

```ts
dsp.reset();
```

`reset()` clears filter and dynamics processing state and restores the default transparent processing configuration.

Destroy the instance:

```ts
dsp.destroy();
```

After `destroy()`, processing and mutation methods are no longer valid.

## Real-time usage

The library is designed around block processing.

Recommended usage:

```text
audio block
    ↓
Buffer
    ↓
dsp.process()
    ↓
Buffer
```

Avoid calling native DSP methods once per sample.

Instead, process complete PCM blocks:

```ts
const output = dsp.process(pcmBlock);
```

The API is synchronous and the DSP instance is single-owner.

Do not concurrently access the same DSP instance from multiple worker threads.

## Native implementation

The native addon uses Node-API/N-API and miniaudio's data-converter infrastructure.

miniaudio is included as a pinned git submodule:

```text
third_party/miniaudio
```

The native layer uses miniaudio for PCM/DSP infrastructure only.

It does not create:

- audio devices
- playback devices
- capture devices
- hardware audio streams

Audio decoding, encoding, playback and device management remain outside this package.

## Building from source

Requirements:

- Node.js `>= 22`
- npm
- Python 3
- C/C++ build toolchain
- Git
- Git submodules

Initialize dependencies:

```bash
git submodule update --init --recursive
```

Install dependencies:

```bash
npm install
```

Run tests:

```bash
npm test
```

The native addon is built as part of the npm build lifecycle.

## Testing

The test suite covers:

- PCM processing
- silence
- sine waves
- impulse responses
- random PCM
- lifecycle and destruction
- volume
- mute
- pan
- clipping protection
- biquad filters
- EQ
- filter graph
- filter ordering
- state preservation
- compressor
- limiter
- soft clipping
- runtime parameter updates
- invalid parameter handling

Run:

```bash
npm test
```

## CI

Every change is intended to be validated across:

- Linux
- macOS
- Windows

and supported Node.js versions.

The CI workflow initializes the miniaudio submodule before building and testing.

## Releases

Published GitHub Releases are built automatically by GitHub Actions.

Release flow:

```text
Git tag / GitHub Release
        │
        ▼
 Release workflow
        │
        ├── Linux
        ├── macOS
        └── Windows
        │
        ▼
 Native build + tests
        │
        ▼
 npm package
        │
        ▼
 GitHub Release assets
```

Release artifacts are generated from the exact source associated with the release tag.

See the project's releases:

https://github.com/zijipia/audio-dsp/releases

## Versioning

The package follows semantic versioning.

The package version and native addon version are kept aligned with the current DSP phase.

Current development line:

```text
@ziji/audio-dsp 0.8.x
```

## Project status

Current implementation:

- Phase 0 — Foundation: complete
- Phase 1 — PCM processing: complete
- Phase 2 — miniaudio integration: complete
- Phase 3 — Basic filters: complete
- Phase 4 — Biquad filters: complete
- Phase 5 — EQ: complete
- Phase 6 — Filter graph: complete
- Phase 7 — Dynamics processing: complete

The project is currently focused on building a stable native DSP foundation for ZiPlayer and other Node.js audio-processing applications.

## Scope

`@ziji/audio-dsp` is a DSP engine.

It is **not**:

- an audio player
- an audio decoder
- an audio encoder
- an FFmpeg replacement
- an audio device abstraction
- a streaming server

Applications are responsible for obtaining PCM audio and sending processed PCM to the appropriate playback/output layer.

## License

See [`LICENSE`](./LICENSE).

## Repository

https://github.com/zijipia/audio-dsp

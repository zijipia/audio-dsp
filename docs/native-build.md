# Native build prerequisites

Phase 0 targets Node.js 22+ and uses node-gyp with a C++17-capable toolchain.

## Linux

- Node.js 22+
- Python 3
- GCC/G++ or Clang with C++17 support
- `make`

## macOS

- Node.js 22+
- Python 3
- Xcode Command Line Tools

## Windows

- Node.js 22+
- Python 3
- Visual Studio 2022 Build Tools with the C++ workload

Run:

```sh
npm install
npm run build
npm test
```

## Miniaudio pin

The project is pinned to **miniaudio v0.11.25** for the native DSP foundation. Phase 0 documents the dependency; vendoring and DSP integration are intentionally deferred to Phase 2.

Reference: https://github.com/mackron/miniaudio/tree/9634bed

This package uses miniaudio for DSP functionality only. It must not enable `ma_device` or take on decoder/device responsibilities.

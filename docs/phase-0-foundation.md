# Phase 0 — Project foundation

Status: **complete**

- [x] Define package name, public API, and supported Node.js versions.
  - Package: `@ziji/audio-dsp`
  - Node.js: `>=22`
  - Initial API types are defined in `src/index.ts`.
- [x] Set up TypeScript source and native C/C++ build structure.
  - `src/`, `native/`, `tsconfig.json`, `binding.gyp`
- [x] Set up N-API bindings.
  - Minimal addon loads through Node-API and exposes a native version smoke function.
- [x] Add development, build, test, and benchmark scripts.
  - `build`, `build:native`, `build:ts`, `test`, `benchmark`
- [x] Add CI for supported platforms.
  - GitHub Actions: Ubuntu, macOS, Windows × Node 22/24.
- [x] Document native build prerequisites.
  - `docs/native-build.md`
- [x] Pin and document the miniaudio version used by the project.
  - miniaudio `v0.11.25`, pinned to commit `9634bed`.
  - Vendoring/integration remains Phase 2 work.

## Scope boundary

Phase 0 establishes the repository/build contract only. `createDSP()` is intentionally not implemented yet; PCM processing starts in Phase 1.

export type AudioFormat = "s16" | "f32";

export interface DSPOptions {
  sampleRate: number;
  channels: number;
  format: AudioFormat;
}

export interface EQBand {
  type: "lowShelf" | "peaking" | "highShelf";
  frequency: number;
  gain: number;
  q?: number;
}

export interface AudioDSP {
  process(input: Buffer, output?: Buffer): Buffer;
  reset(): void;
  destroy(): void;
  setVolume(gain: number): void;
  setEQ(bands: EQBand[]): void;
}

export interface NativeAddon {
  version(): string;
}

// Phase 0 only defines the public surface. DSP behavior is implemented in Phase 1+.
export function createDSP(_options: DSPOptions): AudioDSP {
  throw new Error("createDSP() is not implemented yet; see Phase 1 — PCM processing MVP");
}

export function nativeVersion(): string {
  // eslint-free CommonJS loading keeps the Phase 0 package dependency-light.
  // eslint-disable-next-line @typescript-eslint/no-var-requires
  const native = require("../build/Release/audio_dsp.node") as NativeAddon;
  return native.version();
}

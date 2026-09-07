import { Buffer } from "node:buffer";

export type AudioFormat = "s16" | "f32";

export interface DSPOptions {
  sampleRate: number;
  channels: number;
  format: AudioFormat;
}

export interface AudioDSP {
  process(input: Buffer): Buffer;
  setVolume(volume: number): void;
  setMute(muted: boolean): void;
  setPan(pan: number): void;
  reset(): void;
  destroy(): void;
}

interface NativeDSP {
  process(input: Buffer): Buffer;
  setVolume(volume: number): void;
  setMute(muted: boolean): void;
  setPan(pan: number): void;
  reset(): void;
  destroy(): void;
}

interface NativeAddon {
  version(): string;
  createDSP(options: DSPOptions): NativeDSP;
}

function loadNative(): NativeAddon {
  // eslint-free CommonJS loading keeps the package dependency-light.
  // eslint-disable-next-line @typescript-eslint/no-var-requires
  return require("../build/Release/audio_dsp.node") as NativeAddon;
}

function validateOptions(options: DSPOptions): void {
  if (!Number.isInteger(options.sampleRate) || options.sampleRate <= 0) {
    throw new RangeError("sampleRate must be a positive integer");
  }
  if (!Number.isInteger(options.channels) || options.channels < 1 || options.channels > 8) {
    throw new RangeError("channels must be an integer from 1 to 8");
  }
  if (options.format !== "s16" && options.format !== "f32") {
    throw new TypeError("format must be s16 or f32");
  }
}

export function createDSP(options: DSPOptions): AudioDSP {
  validateOptions(options);
  const native = loadNative().createDSP(options);
  let destroyed = false;

  return {
    process(input: Buffer): Buffer {
      if (destroyed) throw new Error("AudioDSP instance has been destroyed");
      if (!Buffer.isBuffer(input)) throw new TypeError("process() requires a Buffer");
      return native.process(input);
    },
    setVolume(volume: number): void {
      if (destroyed) throw new Error("AudioDSP instance has been destroyed");
      if (!Number.isFinite(volume) || volume < 0 || volume > 4) {
        throw new RangeError("volume must be a finite number from 0 to 4");
      }
      native.setVolume(volume);
    },
    setMute(muted: boolean): void {
      if (destroyed) throw new Error("AudioDSP instance has been destroyed");
      if (typeof muted !== "boolean") throw new TypeError("muted must be a boolean");
      native.setMute(muted);
    },
    setPan(pan: number): void {
      if (destroyed) throw new Error("AudioDSP instance has been destroyed");
      if (!Number.isFinite(pan) || pan < -1 || pan > 1) {
        throw new RangeError("pan must be a finite number from -1 to 1");
      }
      if (options.channels < 2) throw new Error("pan requires at least 2 channels");
      native.setPan(pan);
    },
    reset(): void {
      if (destroyed) throw new Error("AudioDSP instance has been destroyed");
      native.reset();
    },
    destroy(): void {
      if (!destroyed) {
        native.destroy();
        destroyed = true;
      }
    },
  };
}

export function nativeVersion(): string {
  return loadNative().version();
}

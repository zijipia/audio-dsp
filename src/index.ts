import { Buffer } from "node:buffer";

export type AudioFormat = "s16" | "f32";
export type BiquadType = "lowPass" | "highPass" | "bandPass" | "notch" | "peaking" | "lowShelf" | "highShelf";

export interface BiquadConfig {
  type: BiquadType;
  frequency: number;
  q?: number;
  gain?: number;
}

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
  setBiquad(config: BiquadConfig): void;
  clearBiquad(): void;
  reset(): void;
  destroy(): void;
}

interface NativeDSP {
  process(input: Buffer): Buffer;
  setVolume(volume: number): void;
  setMute(muted: boolean): void;
  setPan(pan: number): void;
  setBiquad(config: BiquadConfig): void;
  clearBiquad(): void;
  reset(): void;
  destroy(): void;
}

interface NativeAddon {
  version(): string;
  createDSP(options: DSPOptions): NativeDSP;
}

function loadNative(): NativeAddon {
  // eslint-disable-next-line @typescript-eslint/no-var-requires
  return require("../build/Release/audio_dsp.node") as NativeAddon;
}

function validateOptions(options: DSPOptions): void {
  if (!Number.isInteger(options.sampleRate) || options.sampleRate <= 0) throw new RangeError("sampleRate must be a positive integer");
  if (!Number.isInteger(options.channels) || options.channels < 1 || options.channels > 8) throw new RangeError("channels must be an integer from 1 to 8");
  if (options.format !== "s16" && options.format !== "f32") throw new TypeError("format must be s16 or f32");
}

function validateBiquad(config: BiquadConfig, sampleRate: number): void {
  if (!config || typeof config !== "object") throw new TypeError("biquad config must be an object");
  const types: BiquadType[] = ["lowPass", "highPass", "bandPass", "notch", "peaking", "lowShelf", "highShelf"];
  if (!types.includes(config.type)) throw new TypeError("invalid biquad type");
  if (!Number.isFinite(config.frequency) || config.frequency <= 0 || config.frequency >= sampleRate / 2) throw new RangeError("frequency must be between 0 and Nyquist");
  if (config.q !== undefined && (!Number.isFinite(config.q) || config.q <= 0)) throw new RangeError("q must be a positive finite number");
  if (config.gain !== undefined && (!Number.isFinite(config.gain) || config.gain < -24 || config.gain > 24)) throw new RangeError("gain must be a finite number from -24 to 24 dB");
}

export function createDSP(options: DSPOptions): AudioDSP {
  validateOptions(options);
  const native = loadNative().createDSP(options);
  let destroyed = false;
  const guard = () => { if (destroyed) throw new Error("AudioDSP instance has been destroyed"); };
  return {
    process(input: Buffer): Buffer { guard(); if (!Buffer.isBuffer(input)) throw new TypeError("process() requires a Buffer"); return native.process(input); },
    setVolume(volume: number): void { guard(); if (!Number.isFinite(volume) || volume < 0 || volume > 4) throw new RangeError("volume must be a finite number from 0 to 4"); native.setVolume(volume); },
    setMute(muted: boolean): void { guard(); if (typeof muted !== "boolean") throw new TypeError("muted must be a boolean"); native.setMute(muted); },
    setPan(pan: number): void { guard(); if (!Number.isFinite(pan) || pan < -1 || pan > 1) throw new RangeError("pan must be a finite number from -1 to 1"); if (options.channels < 2) throw new Error("pan requires at least 2 channels"); native.setPan(pan); },
    setBiquad(config: BiquadConfig): void { guard(); validateBiquad(config, options.sampleRate); native.setBiquad(config); },
    clearBiquad(): void { guard(); native.clearBiquad(); },
    reset(): void { guard(); native.reset(); },
    destroy(): void { if (!destroyed) { native.destroy(); destroyed = true; } },
  };
}

export function nativeVersion(): string { return loadNative().version(); }

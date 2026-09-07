import { Buffer } from "node:buffer";

export type AudioFormat = "s16" | "f32";
export type BiquadType = "lowPass" | "highPass" | "bandPass" | "notch" | "peaking" | "lowShelf" | "highShelf";
export type EQBandType = "lowShelf" | "peaking" | "highShelf";

export interface BiquadConfig {
  type: BiquadType;
  frequency: number;
  q?: number;
  gain?: number;
}

export interface EQBand {
  type: EQBandType;
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
  setEQ(bands: EQBand[]): void;
  clearEQ(): void;
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
  setEQ(bands: EQBand[]): void;
  clearEQ(): void;
  reset(): void;
  destroy(): void;
}

interface NativeAddon { version(): string; createDSP(options: DSPOptions): NativeDSP; }
function loadNative(): NativeAddon { return require("../build/Release/audio_dsp.node") as NativeAddon; }
function validateOptions(o: DSPOptions): void { if(!Number.isInteger(o.sampleRate)||o.sampleRate<=0)throw new RangeError("sampleRate must be a positive integer"); if(!Number.isInteger(o.channels)||o.channels<1||o.channels>8)throw new RangeError("channels must be an integer from 1 to 8"); if(o.format!=="s16"&&o.format!=="f32")throw new TypeError("format must be s16 or f32"); }
function validateBiquad(c: BiquadConfig,sr:number):void{const t:BiquadType[]=["lowPass","highPass","bandPass","notch","peaking","lowShelf","highShelf"];if(!c||typeof c!=="object"||!t.includes(c.type))throw new TypeError("invalid biquad type");if(!Number.isFinite(c.frequency)||c.frequency<=0||c.frequency>=sr/2)throw new RangeError("frequency must be between 0 and Nyquist");if(c.q!==undefined&&(!Number.isFinite(c.q)||c.q<=0))throw new RangeError("q must be a positive finite number");if(c.gain!==undefined&&(!Number.isFinite(c.gain)||c.gain<-24||c.gain>24))throw new RangeError("gain must be a finite number from -24 to 24 dB");}
function validateEQBand(b:EQBand,sr:number):void{if(!b||typeof b!=="object")throw new TypeError("EQ band must be an object");if(b.type!=="lowShelf"&&b.type!=="peaking"&&b.type!=="highShelf")throw new TypeError("invalid EQ band type");if(!Number.isFinite(b.frequency)||b.frequency<=0||b.frequency>=sr/2)throw new RangeError("EQ frequency must be between 0 and Nyquist");if(b.q!==undefined&&(!Number.isFinite(b.q)||b.q<=0))throw new RangeError("EQ q must be a positive finite number");if(b.gain===undefined||!Number.isFinite(b.gain)||b.gain<-24||b.gain>24)throw new RangeError("EQ gain must be a finite number from -24 to 24 dB");}
export function createDSP(options:DSPOptions):AudioDSP{validateOptions(options);const n=loadNative().createDSP(options);let destroyed=false;const guard=()=>{if(destroyed)throw new Error("AudioDSP instance has been destroyed")};return{process(i:Buffer){guard();if(!Buffer.isBuffer(i))throw new TypeError("process() requires a Buffer");return n.process(i)},setVolume(v:number){guard();if(!Number.isFinite(v)||v<0||v>4)throw new RangeError("volume must be a finite number from 0 to 4");n.setVolume(v)},setMute(v:boolean){guard();if(typeof v!=="boolean")throw new TypeError("muted must be a boolean");n.setMute(v)},setPan(v:number){guard();if(!Number.isFinite(v)||v<-1||v>1)throw new RangeError("pan must be a finite number from -1 to 1");if(options.channels<2)throw new Error("pan requires at least 2 channels");n.setPan(v)},setBiquad(c:BiquadConfig){guard();validateBiquad(c,options.sampleRate);n.setBiquad(c)},clearBiquad(){guard();n.clearBiquad()},setEQ(bands:EQBand[]){guard();if(!Array.isArray(bands)||bands.length>16)throw new RangeError("EQ bands must be an array with at most 16 bands");bands.forEach(b=>validateEQBand(b,options.sampleRate));n.setEQ(bands)},clearEQ(){guard();n.clearEQ()},reset(){guard();n.reset()},destroy(){if(!destroyed){n.destroy();destroyed=true}}}}
export function nativeVersion():string{return loadNative().version();}

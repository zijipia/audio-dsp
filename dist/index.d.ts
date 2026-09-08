import { Buffer } from "node:buffer";
export type AudioFormat = "s16" | "f32";
export type BiquadType = "lowPass" | "highPass" | "bandPass" | "notch" | "peaking" | "lowShelf" | "highShelf";
export type EQBandType = "lowShelf" | "peaking" | "highShelf";
export type FilterType = "biquad" | "eq";
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
export interface FilterNode {
    id: string;
    type: FilterType;
}
export interface LimiterConfig {
    threshold?: number;
    release?: number;
}
export interface CompressorConfig {
    threshold?: number;
    ratio?: number;
    attack?: number;
    release?: number;
}
export interface DSPOptions {
    sampleRate: number;
    channels: number;
    format: AudioFormat;
}
export interface AudioDSP {
    process(i: Buffer): Buffer;
    setVolume(v: number): void;
    setMute(v: boolean): void;
    setPan(v: number): void;
    setBiquad(c: BiquadConfig): void;
    clearBiquad(): void;
    setEQ(b: EQBand[]): void;
    clearEQ(): void;
    addBiquad(id: string, c: BiquadConfig): void;
    addEQ(id: string, b: EQBand[]): void;
    removeFilter(id: string): void;
    setFilterOrder(ids: string[]): void;
    updateBiquad(id: string, c: BiquadConfig): void;
    updateEQ(id: string, b: EQBand[]): void;
    setLimiter(c: LimiterConfig): void;
    clearLimiter(): void;
    setCompressor(c: CompressorConfig): void;
    clearCompressor(): void;
    setSoftClip(enabled: boolean, drive?: number): void;
    resetState(): void;
    reset(): void;
    destroy(): void;
}
export interface NativeDecoder {
    read(frames: number): Buffer;
    seek(frame: number): void;
    cursor(): number;
    length(): number;
    sampleRate(): number;
    channels(): number;
    destroy(): void;
}
export declare function createDecoder(input: Buffer | string): NativeDecoder;
export declare function createDSP(o: DSPOptions): AudioDSP;
export declare function nativeVersion(): string;

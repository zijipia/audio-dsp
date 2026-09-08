"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.createDecoder = createDecoder;
exports.createDSP = createDSP;
exports.nativeVersion = nativeVersion;
const node_buffer_1 = require("node:buffer");
function loadNative() {
    return require("../build/Release/audio_dsp.node");
}
function loadDecoder() {
    return require("../build/Release/audio_decoder.node");
}
function createDecoder(input) {
    if (!node_buffer_1.Buffer.isBuffer(input) && typeof input !== "string")
        throw new TypeError("createDecoder() requires an encoded Buffer or file path");
    if (typeof input === "string" && input.length === 0)
        throw new RangeError("file path must not be empty");
    if (node_buffer_1.Buffer.isBuffer(input) && input.length === 0)
        throw new RangeError("encoded Buffer must not be empty");
    return new (loadDecoder().NativeDecoder)(input);
}
const BT = [
    "lowPass",
    "highPass",
    "bandPass",
    "notch",
    "peaking",
    "lowShelf",
    "highShelf",
];
function validateOptions(o) {
    if (!Number.isInteger(o.sampleRate) || o.sampleRate <= 0)
        throw new RangeError("sampleRate must be a positive integer");
    if (!Number.isInteger(o.channels) || o.channels < 1 || o.channels > 8)
        throw new RangeError("channels must be an integer from 1 to 8");
    if (o.format !== "s16" && o.format !== "f32")
        throw new TypeError("format must be s16 or f32");
}
function validateBiquad(c, sr) {
    if (!c || !BT.includes(c.type))
        throw new TypeError("invalid biquad type");
    if (!Number.isFinite(c.frequency) ||
        c.frequency <= 0 ||
        c.frequency >= sr / 2)
        throw new RangeError("frequency must be between 0 and Nyquist");
    if (c.q !== undefined && (!Number.isFinite(c.q) || c.q <= 0))
        throw new RangeError("q must be positive");
    if (c.gain !== undefined &&
        (!Number.isFinite(c.gain) || c.gain < -24 || c.gain > 24))
        throw new RangeError("gain must be between -24 and 24 dB");
}
function validateEQ(b, sr) {
    if (!Array.isArray(b) || b.length > 16)
        throw new RangeError("EQ bands must be an array with at most 16 bands");
    for (const x of b) {
        if (!x || !["lowShelf", "peaking", "highShelf"].includes(x.type))
            throw new TypeError("invalid EQ band type");
        if (!Number.isFinite(x.frequency) ||
            x.frequency <= 0 ||
            x.frequency >= sr / 2)
            throw new RangeError("EQ frequency must be between 0 and Nyquist");
        if (x.q !== undefined && (!Number.isFinite(x.q) || x.q <= 0))
            throw new RangeError("EQ q must be positive");
        if (x.gain === undefined ||
            !Number.isFinite(x.gain) ||
            x.gain < -24 ||
            x.gain > 24)
            throw new RangeError("EQ gain must be between -24 and 24 dB");
    }
}
function validId(id) {
    if (typeof id !== "string" || !id.trim())
        throw new TypeError("filter id must be a non-empty string");
}
function validLimiter(c) {
    const t = c.threshold ?? -1, r = c.release ?? 50;
    if (!Number.isFinite(t) || t > 0 || t < -24 || !Number.isFinite(r) || r <= 0)
        throw new RangeError("invalid limiter parameters");
}
function validComp(c) {
    const t = c.threshold ?? -18, r = c.ratio ?? 4, a = c.attack ?? 10, rel = c.release ?? 100;
    if (!Number.isFinite(t) ||
        t > 0 ||
        t < -60 ||
        !Number.isFinite(r) ||
        r < 1 ||
        !Number.isFinite(a) ||
        a <= 0 ||
        !Number.isFinite(rel) ||
        rel <= 0)
        throw new RangeError("invalid compressor parameters");
}
function createDSP(o) {
    validateOptions(o);
    const n = loadNative().createDSP(o);
    let dead = false;
    const ids = new Set();
    const guard = () => {
        if (dead)
            throw new Error("AudioDSP instance has been destroyed");
    };
    return {
        process(i) {
            guard();
            if (!node_buffer_1.Buffer.isBuffer(i))
                throw new TypeError("process() requires a Buffer");
            return n.process(i);
        },
        setVolume(v) {
            guard();
            if (!Number.isFinite(v) || v < 0 || v > 4)
                throw new RangeError("volume must be between 0 and 4");
            n.setVolume(v);
        },
        setMute(v) {
            guard();
            if (typeof v !== "boolean")
                throw new TypeError("muted must be boolean");
            n.setMute(v);
        },
        setPan(v) {
            guard();
            if (!Number.isFinite(v) || v < -1 || v > 1)
                throw new RangeError("pan must be between -1 and 1");
            if (o.channels < 2)
                throw new Error("pan requires at least 2 channels");
            n.setPan(v);
        },
        setBiquad(c) {
            guard();
            validateBiquad(c, o.sampleRate);
            n.setBiquad(c);
        },
        clearBiquad() {
            guard();
            n.clearBiquad();
        },
        setEQ(b) {
            guard();
            validateEQ(b, o.sampleRate);
            n.setEQ(b);
        },
        clearEQ() {
            guard();
            n.clearEQ();
        },
        addBiquad(id, c) {
            guard();
            validId(id);
            if (ids.has(id))
                throw new Error(`filter id already exists: ${id}`);
            validateBiquad(c, o.sampleRate);
            n.addBiquad(id, c);
            ids.add(id);
        },
        addEQ(id, b) {
            guard();
            validId(id);
            if (ids.has(id))
                throw new Error(`filter id already exists: ${id}`);
            validateEQ(b, o.sampleRate);
            n.addEQ(id, b);
            ids.add(id);
        },
        removeFilter(id) {
            guard();
            validId(id);
            if (!ids.has(id))
                throw new Error(`unknown filter id: ${id}`);
            n.removeFilter(id);
            ids.delete(id);
        },
        setFilterOrder(order) {
            guard();
            if (!Array.isArray(order) ||
                order.length !== ids.size ||
                new Set(order).size !== order.length ||
                order.some((x) => !ids.has(x)))
                throw new RangeError("filter order must contain every filter id exactly once");
            n.setFilterOrder(order);
        },
        updateBiquad(id, c) {
            guard();
            validId(id);
            if (!ids.has(id))
                throw new Error(`unknown filter id: ${id}`);
            validateBiquad(c, o.sampleRate);
            n.updateBiquad(id, c);
        },
        updateEQ(id, b) {
            guard();
            validId(id);
            if (!ids.has(id))
                throw new Error(`unknown filter id: ${id}`);
            validateEQ(b, o.sampleRate);
            n.updateEQ(id, b);
        },
        setLimiter(c) {
            guard();
            validLimiter(c);
            n.setLimiter(c);
        },
        clearLimiter() {
            guard();
            n.clearLimiter();
        },
        setCompressor(c) {
            guard();
            validComp(c);
            n.setCompressor(c);
        },
        clearCompressor() {
            guard();
            n.clearCompressor();
        },
        setSoftClip(en, drive = 2) {
            guard();
            if (typeof en !== "boolean")
                throw new TypeError("enabled must be boolean");
            if (!Number.isFinite(drive) || drive <= 0 || drive > 20)
                throw new RangeError("soft clip drive must be between 0 and 20");
            n.setSoftClip({ enabled: en, drive });
        },
        resetState() {
            guard();
            n.resetState();
        },
        reset() {
            guard();
            n.reset();
            ids.clear();
        },
        destroy() {
            if (!dead) {
                n.destroy();
                dead = true;
                ids.clear();
            }
        },
    };
}
function nativeVersion() {
    return loadNative().version();
}

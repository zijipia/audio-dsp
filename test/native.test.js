const test = require("node:test");
const assert = require("node:assert/strict");
const { createDSP, nativeVersion } = require("../dist/index.js");

function makeDSP(format = "s16", channels = 2) {
  return createDSP({ sampleRate: 48000, channels, format });
}

test("native addon loads through N-API and Phase 3 basic-filter build", () => {
  assert.equal(nativeVersion(), "0.4.0-phase3-basic-filters");
});

test("default processing is transparent", () => {
  const dsp = makeDSP("s16");
  const input = Buffer.alloc(960 * 2 * 2);
  for (let i = 0; i < input.length; i += 1) input[i] = (i * 37 + 11) & 0xff;
  assert.deepEqual(dsp.process(input), input);
  dsp.destroy();
});

test("volume scales s16 samples and clips safely", () => {
  const dsp = makeDSP("s16", 1);
  const input = Buffer.alloc(3 * 2);
  input.writeInt16LE(1000, 0);
  input.writeInt16LE(-1000, 2);
  input.writeInt16LE(20000, 4);
  dsp.setVolume(2);
  const output = dsp.process(input);
  assert.equal(output.readInt16LE(0), 2000);
  assert.equal(output.readInt16LE(2), -2000);
  assert.equal(output.readInt16LE(4), 32767);
  dsp.destroy();
});

test("volume scales f32 samples and clamps to [-1, 1]", () => {
  const dsp = makeDSP("f32", 1);
  const input = Buffer.alloc(3 * 4);
  const view = new Float32Array(input.buffer, input.byteOffset, 3);
  view.set([0.25, -0.5, 0.75]);
  dsp.setVolume(2);
  const output = dsp.process(input);
  const result = new Float32Array(output.buffer, output.byteOffset, 3);
  assert.deepEqual(Array.from(result), [0.5, -1, 1]);
  dsp.destroy();
});

test("mute silences audio and can be toggled at runtime", () => {
  const dsp = makeDSP("s16", 2);
  const input = Buffer.alloc(4 * 2, 0);
  input.writeInt16LE(1200, 0);
  input.writeInt16LE(-800, 2);
  dsp.setMute(true);
  assert.deepEqual(dsp.process(input), Buffer.alloc(input.length));
  dsp.setMute(false);
  assert.deepEqual(dsp.process(input), input);
  dsp.destroy();
});

test("pan applies linear stereo balance without rebuilding the DSP", () => {
  const dsp = makeDSP("s16", 2);
  const input = Buffer.alloc(2 * 2);
  input.writeInt16LE(1000, 0);
  input.writeInt16LE(1000, 2);
  dsp.setPan(-1);
  let output = dsp.process(input);
  assert.equal(output.readInt16LE(0), 1000);
  assert.equal(output.readInt16LE(2), 0);
  dsp.setPan(1);
  output = dsp.process(input);
  assert.equal(output.readInt16LE(0), 0);
  assert.equal(output.readInt16LE(2), 1000);
  dsp.destroy();
});

test("pan is rejected for mono and invalid ranges", () => {
  const mono = makeDSP("f32", 1);
  assert.throws(() => mono.setPan(0), /at least 2 channels/);
  mono.destroy();

  const dsp = makeDSP("f32", 2);
  assert.throws(() => dsp.setPan(-1.01), /pan/);
  assert.throws(() => dsp.setPan(1.01), /pan/);
  assert.throws(() => dsp.setVolume(-0.01), /volume/);
  assert.throws(() => dsp.setVolume(4.01), /volume/);
  assert.throws(() => dsp.setMute("yes"), /boolean/);
  dsp.destroy();
});

test("reset restores basic-filter defaults", () => {
  const dsp = makeDSP("s16", 2);
  const input = Buffer.alloc(2 * 2);
  input.writeInt16LE(1000, 0);
  input.writeInt16LE(1000, 2);
  dsp.setVolume(0.5);
  dsp.setMute(true);
  dsp.setPan(-1);
  dsp.reset();
  assert.deepEqual(dsp.process(input), input);
  dsp.destroy();
});

test("existing frame validation and lifecycle behavior remain intact", () => {
  const dsp = makeDSP("s16");
  assert.throws(() => dsp.process(Buffer.alloc(3)), /aligned/);
  const input = Buffer.alloc(480 * 4, 7);
  dsp.reset();
  dsp.reset();
  assert.deepEqual(dsp.process(input), input);
  dsp.destroy();
  dsp.destroy();
  assert.throws(() => dsp.process(input), /destroyed/);
  assert.throws(() => dsp.reset(), /destroyed/);
  assert.throws(() => dsp.setVolume(1), /destroyed/);
});

test("options and empty blocks are validated", () => {
  assert.throws(() => createDSP({ sampleRate: 0, channels: 2, format: "s16" }), /sampleRate/);
  assert.throws(() => createDSP({ sampleRate: 48000, channels: 0, format: "s16" }), /channels/);
  assert.throws(() => createDSP({ sampleRate: 48000, channels: 2, format: "u8" }), /format/);

  const dsp = makeDSP("f32");
  assert.deepEqual(dsp.process(Buffer.alloc(0)), Buffer.alloc(0));
  dsp.destroy();
});

test("repeated create/process/destroy cycles complete without lifecycle errors", () => {
  const input = Buffer.alloc(480 * 4, 11);
  for (let i = 0; i < 500; i += 1) {
    const dsp = makeDSP("s16");
    assert.deepEqual(dsp.process(input), input);
    dsp.setVolume(0.75);
    dsp.setMute(false);
    dsp.setPan(i % 2 === 0 ? -0.25 : 0.25);
    dsp.reset();
    dsp.destroy();
  }
});

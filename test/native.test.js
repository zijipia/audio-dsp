const test = require("node:test");
const assert = require("node:assert/strict");
const { createDSP, nativeVersion } = require("../dist/index.js");

function makeDSP(format = "s16") {
  return createDSP({ sampleRate: 48000, channels: 2, format });
}

test("native addon loads through N-API", () => {
  assert.equal(nativeVersion(), "0.2.0-phase1");
});

test("silence is preserved for s16 block processing", () => {
  const dsp = makeDSP("s16");
  const input = Buffer.alloc(960 * 2 * 2);
  const output = dsp.process(input);
  assert.deepEqual(output, input);
  assert.notStrictEqual(output, input);
  dsp.destroy();
});

test("sine-wave samples are preserved for f32 block processing", () => {
  const dsp = makeDSP("f32");
  const input = Buffer.alloc(480 * 2 * 4);
  const view = new Float32Array(input.buffer, input.byteOffset, input.byteLength / 4);
  for (let i = 0; i < view.length; i += 1) view[i] = Math.sin((2 * Math.PI * i) / 32);
  const output = dsp.process(input);
  assert.deepEqual(output, input);
  dsp.destroy();
});

test("impulse is preserved and frame alignment is enforced", () => {
  const dsp = makeDSP("s16");
  const input = Buffer.alloc(480 * 4);
  input.writeInt16LE(32767, 0);
  const output = dsp.process(input);
  assert.deepEqual(output, input);
  assert.throws(() => dsp.process(Buffer.alloc(3)), /aligned/);
  dsp.destroy();
});

test("random PCM is preserved without per-sample JS calls", () => {
  const dsp = makeDSP("s16");
  const input = Buffer.alloc(960 * 4);
  for (let i = 0; i < input.length; i += 1) input[i] = (i * 73 + 19) & 0xff;
  assert.deepEqual(dsp.process(input), input);
  dsp.destroy();
});

test("reset is repeatable and destroy is idempotent", () => {
  const dsp = makeDSP("s16");
  const input = Buffer.alloc(480 * 4, 7);
  assert.deepEqual(dsp.process(input), input);
  dsp.reset();
  dsp.reset();
  assert.deepEqual(dsp.process(input), input);
  dsp.destroy();
  dsp.destroy();
  assert.throws(() => dsp.process(input), /destroyed/);
  assert.throws(() => dsp.reset(), /destroyed/);
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
    dsp.reset();
    dsp.destroy();
  }
});

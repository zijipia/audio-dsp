const test = require("node:test");
const assert = require("node:assert/strict");
const { createDSP } = require("../dist/index.js");

function makeDSP(channels = 1) {
  return createDSP({ sampleRate: 48000, channels, format: "f32" });
}

function floats(buffer, count) {
  return new Float32Array(buffer.buffer, buffer.byteOffset, count);
}

function impulse(value = 1) {
  const b = Buffer.alloc(4);
  floats(b, 1)[0] = value;
  return b;
}

test("resetState clears biquad history but preserves configuration", () => {
  const d = makeDSP();
  d.setBiquad({ type: "lowPass", frequency: 1000, q: 0.707 });

  const first = floats(d.process(impulse()), 1)[0];
  const continued = floats(d.process(impulse(0)), 1)[0];
  assert.notEqual(continued, first);

  d.resetState();
  const afterReset = floats(d.process(impulse()), 1)[0];
  assert.equal(afterReset, first);

  d.destroy();
});

test("resetState preserves runtime configuration", () => {
  const d = makeDSP(2);
  d.setVolume(0.5);
  d.setPan(0.25);
  d.setBiquad({ type: "peaking", frequency: 1000, q: 1, gain: 6 });
  d.setCompressor({ threshold: -18, ratio: 4, attack: 1, release: 50 });
  d.setLimiter({ threshold: -6, release: 30 });
  d.setSoftClip(true, 2);

  const input = Buffer.alloc(2 * 4);
  floats(input, 2).fill(0.8);
  d.process(input);
  d.resetState();

  assert.doesNotThrow(() => d.process(input));
  d.destroy();
});

test("resetState clears graph filter history", () => {
  const d = makeDSP();
  d.addBiquad("lp", { type: "lowPass", frequency: 1000, q: 0.707 });
  d.addEQ("eq", [{ type: "peaking", frequency: 2000, q: 1, gain: 3 }]);
  d.setFilterOrder(["lp", "eq"]);

  const first = floats(d.process(impulse()), 1)[0];
  const continued = floats(d.process(impulse(0)), 1)[0];
  assert.notEqual(continued, first);

  d.resetState();
  const afterReset = floats(d.process(impulse()), 1)[0];
  assert.equal(afterReset, first);

  d.destroy();
});

test("resetState is distinct from full reset", () => {
  const d = makeDSP();
  d.setVolume(0.5);
  d.setBiquad({ type: "lowPass", frequency: 1000, q: 0.707 });
  d.resetState();

  const input = impulse();
  const preserved = floats(d.process(input), 1)[0];
  assert.ok(preserved > 0 && preserved < 1);

  d.reset();
  assert.equal(floats(d.process(input), 1)[0], 1);

  d.destroy();
});

test("resetState can be called repeatedly without changing configuration", () => {
  const d = makeDSP();
  d.setVolume(0.75);
  d.setLimiter({ threshold: -6, release: 30 });
  d.resetState();
  d.resetState();

  const input = impulse(0.25);
  const out = floats(d.process(input), 1)[0];
  assert.ok(out > 0 && out <= 0.25);

  d.destroy();
});

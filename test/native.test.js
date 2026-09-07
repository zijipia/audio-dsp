const test = require("node:test");
const assert = require("node:assert/strict");

const native = require("../build/Release/audio_dsp.node");

test("native addon loads through N-API", () => {
  assert.equal(typeof native.version, "function");
  assert.equal(native.version(), "0.1.0-phase0");
});

const { performance } = require("node:perf_hooks");
const native = require("../build/Release/audio_dsp.node");

const start = performance.now();
for (let i = 0; i < 100000; i += 1) native.version();
const elapsedMs = performance.now() - start;

console.log(JSON.stringify({
  operation: "native-addon-call-overhead-smoke",
  iterations: 100000,
  elapsedMs,
  callsPerSecond: 100000 / (elapsedMs / 1000)
}, null, 2));

#include <napi.h>

#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>

namespace {

struct DSPContext {
  uint32_t sample_rate;
  uint32_t channels;
  uint32_t bytes_per_sample;
  bool destroyed;
};

DSPContext* RequireContext(const Napi::CallbackInfo& info) {
  auto* ctx = info.This().Get("_context").As<Napi::External<DSPContext>>().Data();
  if (ctx == nullptr || ctx->destroyed) {
    throw Napi::Error::New(info.Env(), "AudioDSP instance has been destroyed");
  }
  return ctx;
}

Napi::Value Process(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  DSPContext* ctx = RequireContext(info);

  if (info.Length() < 1 || !info[0].IsBuffer()) {
    throw Napi::TypeError::New(env, "process() requires a Buffer");
  }

  Napi::Buffer<uint8_t> input = info[0].As<Napi::Buffer<uint8_t>>();
  const size_t frame_bytes = static_cast<size_t>(ctx->channels) * ctx->bytes_per_sample;
  if (frame_bytes == 0 || input.Length() % frame_bytes != 0) {
    throw Napi::RangeError::New(env, "input buffer is not aligned to complete audio frames");
  }

  const size_t frames = input.Length() / frame_bytes;
  if (frames > std::numeric_limits<uint32_t>::max()) {
    throw Napi::RangeError::New(env, "input contains too many frames");
  }

  Napi::Buffer<uint8_t> output = Napi::Buffer<uint8_t>::New(env, input.Length());
  if (input.Length() > 0) {
    std::memcpy(output.Data(), input.Data(), input.Length());
  }
  return output;
}

Napi::Value Reset(const Napi::CallbackInfo& info) {
  RequireContext(info);
  return info.Env().Undefined();
}

Napi::Value Destroy(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto external = info.This().Get("_context").As<Napi::External<DSPContext>>();
  DSPContext* ctx = external.Data();
  if (ctx != nullptr) {
    ctx->destroyed = true;
  }
  return env.Undefined();
}

void FinalizeContext(Napi::Env, DSPContext* ctx) {
  delete ctx;
}

Napi::Value CreateDSP(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1 || !info[0].IsObject()) {
    throw Napi::TypeError::New(env, "createDSP() requires an options object");
  }

  Napi::Object options = info[0].As<Napi::Object>();
  Napi::Value sample_rate_value = options.Get("sampleRate");
  Napi::Value channels_value = options.Get("channels");
  Napi::Value format_value = options.Get("format");

  if (!sample_rate_value.IsNumber() || !channels_value.IsNumber() || !format_value.IsString()) {
    throw Napi::TypeError::New(env, "sampleRate, channels, and format are required");
  }

  const double sample_rate = sample_rate_value.As<Napi::Number>().DoubleValue();
  const double channels = channels_value.As<Napi::Number>().DoubleValue();
  const std::string format = format_value.As<Napi::String>().Utf8Value();

  if (!(sample_rate > 0) || sample_rate > std::numeric_limits<uint32_t>::max() || sample_rate != static_cast<uint32_t>(sample_rate)) {
    throw Napi::RangeError::New(env, "sampleRate must be a positive integer");
  }
  if (!(channels >= 1) || channels > 8 || channels != static_cast<uint32_t>(channels)) {
    throw Napi::RangeError::New(env, "channels must be an integer from 1 to 8");
  }
  if (format != "s16" && format != "f32") {
    throw Napi::TypeError::New(env, "format must be s16 or f32");
  }

  auto* ctx = new DSPContext{
      static_cast<uint32_t>(sample_rate),
      static_cast<uint32_t>(channels),
      format == "s16" ? 2u : 4u,
      false};

  Napi::Object dsp = Napi::Object::New(env);
  dsp.Set("_context", Napi::External<DSPContext>::New(env, ctx, FinalizeContext));
  dsp.Set("process", Napi::Function::New(env, Process));
  dsp.Set("reset", Napi::Function::New(env, Reset));
  dsp.Set("destroy", Napi::Function::New(env, Destroy));
  return dsp;
}

Napi::Value Version(const Napi::CallbackInfo& info) {
  return Napi::String::New(info.Env(), "0.2.0-phase1");
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("version", Napi::Function::New(env, Version));
  exports.Set("createDSP", Napi::Function::New(env, CreateDSP));
  return exports;
}

}  // namespace

NODE_API_MODULE(audio_dsp, Init)

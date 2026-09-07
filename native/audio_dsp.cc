#include <napi.h>

#include "miniaudio.h"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

struct DSPContext {
  uint32_t sample_rate;
  uint32_t channels;
  uint32_t bytes_per_sample;
  ma_format format;
  ma_data_converter converter;
  bool converter_initialized;
  bool destroyed;
};

DSPContext* RequireContext(const Napi::CallbackInfo& info) {
  auto* ctx = info.This().Get("_context").As<Napi::External<DSPContext>>().Data();
  if (ctx == nullptr || ctx->destroyed) {
    throw Napi::Error::New(info.Env(), "AudioDSP instance has been destroyed");
  }
  return ctx;
}

void InitConverter(DSPContext* ctx) {
  const ma_data_converter_config config = ma_data_converter_config_init(
      ctx->format,
      ctx->format,
      ctx->channels,
      ctx->channels,
      ctx->sample_rate,
      ctx->sample_rate);

  const ma_result result = ma_data_converter_init(&config, nullptr, &ctx->converter);
  if (result != MA_SUCCESS) {
    throw std::runtime_error("miniaudio data converter initialization failed");
  }
  ctx->converter_initialized = true;
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
  if (frames > std::numeric_limits<ma_uint64>::max()) {
    throw Napi::RangeError::New(env, "input contains too many frames");
  }

  Napi::Buffer<uint8_t> output = Napi::Buffer<uint8_t>::New(env, input.Length());
  if (frames == 0) {
    return output;
  }

  ma_uint64 input_frames = static_cast<ma_uint64>(frames);
  ma_uint64 output_frames = static_cast<ma_uint64>(frames);
  const ma_result result = ma_data_converter_process_pcm_frames(
      &ctx->converter,
      input.Data(),
      &input_frames,
      output.Data(),
      &output_frames);
  if (result != MA_SUCCESS) {
    throw Napi::Error::New(env, "miniaudio failed to process PCM frames");
  }
  if (input_frames != frames || output_frames != frames) {
    throw Napi::Error::New(env, "miniaudio processed an unexpected frame count");
  }

  return output;
}

Napi::Value Reset(const Napi::CallbackInfo& info) {
  DSPContext* ctx = RequireContext(info);

  if (ctx->converter_initialized) {
    ma_data_converter_uninit(&ctx->converter, nullptr);
    ctx->converter_initialized = false;
  }
  InitConverter(ctx);
  return info.Env().Undefined();
}

Napi::Value Destroy(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto external = info.This().Get("_context").As<Napi::External<DSPContext>>();
  DSPContext* ctx = external.Data();
  if (ctx != nullptr && !ctx->destroyed) {
    if (ctx->converter_initialized) {
      ma_data_converter_uninit(&ctx->converter, nullptr);
      ctx->converter_initialized = false;
    }
    ctx->destroyed = true;
  }
  return env.Undefined();
}

void FinalizeContext(Napi::Env, DSPContext* ctx) {
  if (ctx != nullptr) {
    if (ctx->converter_initialized) {
      ma_data_converter_uninit(&ctx->converter, nullptr);
    }
    delete ctx;
  }
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
      format == "s16" ? ma_format_s16 : ma_format_f32,
      {},
      false,
      false};

  try {
    InitConverter(ctx);
  } catch (const std::exception& error) {
    delete ctx;
    throw Napi::Error::New(env, error.what());
  }

  Napi::Object dsp = Napi::Object::New(env);
  dsp.Set("_context", Napi::External<DSPContext>::New(env, ctx, FinalizeContext));
  dsp.Set("process", Napi::Function::New(env, Process));
  dsp.Set("reset", Napi::Function::New(env, Reset));
  dsp.Set("destroy", Napi::Function::New(env, Destroy));
  return dsp;
}

Napi::Value Version(const Napi::CallbackInfo& info) {
  return Napi::String::New(info.Env(), "0.3.0-phase2-miniaudio");
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("version", Napi::Function::New(env, Version));
  exports.Set("createDSP", Napi::Function::New(env, CreateDSP));
  return exports;
}

}  // namespace

NODE_API_MODULE(audio_dsp, Init)

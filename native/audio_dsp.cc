#include <napi.h>

#include "miniaudio.h"

#include <algorithm>
#include <cmath>
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
  float volume;
  bool muted;
  float pan;
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

float LeftGain(float pan) {
  return pan > 0.0f ? 1.0f - pan : 1.0f;
}

float RightGain(float pan) {
  return pan < 0.0f ? 1.0f + pan : 1.0f;
}

float ClampFloat(float value) {
  return std::max(-1.0f, std::min(1.0f, value));
}

int16_t ScaleS16(int16_t sample, float gain) {
  const float scaled = static_cast<float>(sample) * gain;
  const float clamped = std::max(-32768.0f, std::min(32767.0f, scaled));
  return static_cast<int16_t>(std::lrintf(clamped));
}

void ApplyFilters(DSPContext* ctx, uint8_t* data, size_t frames) {
  const float gain = ctx->muted ? 0.0f : ctx->volume;
  const float left_gain = gain * LeftGain(ctx->pan);
  const float right_gain = gain * RightGain(ctx->pan);

  if (ctx->format == ma_format_s16) {
    auto* samples = reinterpret_cast<int16_t*>(data);
    for (size_t frame = 0; frame < frames; ++frame) {
      const size_t offset = frame * ctx->channels;
      if (ctx->channels == 1) {
        samples[offset] = ScaleS16(samples[offset], gain);
        continue;
      }
      samples[offset] = ScaleS16(samples[offset], left_gain);
      samples[offset + 1] = ScaleS16(samples[offset + 1], right_gain);
      for (uint32_t channel = 2; channel < ctx->channels; ++channel) {
        samples[offset + channel] = ScaleS16(samples[offset + channel], gain);
      }
    }
    return;
  }

  auto* samples = reinterpret_cast<float*>(data);
  for (size_t frame = 0; frame < frames; ++frame) {
    const size_t offset = frame * ctx->channels;
    if (ctx->channels == 1) {
      samples[offset] = ClampFloat(samples[offset] * gain);
      continue;
    }
    samples[offset] = ClampFloat(samples[offset] * left_gain);
    samples[offset + 1] = ClampFloat(samples[offset + 1] * right_gain);
    for (uint32_t channel = 2; channel < ctx->channels; ++channel) {
      samples[offset + channel] = ClampFloat(samples[offset + channel] * gain);
    }
  }
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

  ApplyFilters(ctx, output.Data(), frames);
  return output;
}

Napi::Value SetVolume(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  DSPContext* ctx = RequireContext(info);
  if (info.Length() < 1 || !info[0].IsNumber()) {
    throw Napi::TypeError::New(env, "setVolume() requires a number");
  }
  const double value = info[0].As<Napi::Number>().DoubleValue();
  if (!std::isfinite(value) || value < 0.0 || value > 4.0) {
    throw Napi::RangeError::New(env, "volume must be a finite number from 0 to 4");
  }
  ctx->volume = static_cast<float>(value);
  return env.Undefined();
}

Napi::Value SetMute(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  DSPContext* ctx = RequireContext(info);
  if (info.Length() < 1 || !info[0].IsBoolean()) {
    throw Napi::TypeError::New(env, "setMute() requires a boolean");
  }
  ctx->muted = info[0].As<Napi::Boolean>().Value();
  return env.Undefined();
}

Napi::Value SetPan(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  DSPContext* ctx = RequireContext(info);
  if (info.Length() < 1 || !info[0].IsNumber()) {
    throw Napi::TypeError::New(env, "setPan() requires a number");
  }
  if (ctx->channels < 2) {
    throw Napi::Error::New(env, "pan requires at least 2 channels");
  }
  const double value = info[0].As<Napi::Number>().DoubleValue();
  if (!std::isfinite(value) || value < -1.0 || value > 1.0) {
    throw Napi::RangeError::New(env, "pan must be a finite number from -1 to 1");
  }
  ctx->pan = static_cast<float>(value);
  return env.Undefined();
}

Napi::Value Reset(const Napi::CallbackInfo& info) {
  DSPContext* ctx = RequireContext(info);

  if (ctx->converter_initialized) {
    ma_data_converter_uninit(&ctx->converter, nullptr);
    ctx->converter_initialized = false;
  }
  InitConverter(ctx);
  ctx->volume = 1.0f;
  ctx->muted = false;
  ctx->pan = 0.0f;
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
      false,
      1.0f,
      false,
      0.0f};

  try {
    InitConverter(ctx);
  } catch (const std::exception& error) {
    delete ctx;
    throw Napi::Error::New(env, error.what());
  }

  Napi::Object dsp = Napi::Object::New(env);
  dsp.Set("_context", Napi::External<DSPContext>::New(env, ctx, FinalizeContext));
  dsp.Set("process", Napi::Function::New(env, Process));
  dsp.Set("setVolume", Napi::Function::New(env, SetVolume));
  dsp.Set("setMute", Napi::Function::New(env, SetMute));
  dsp.Set("setPan", Napi::Function::New(env, SetPan));
  dsp.Set("reset", Napi::Function::New(env, Reset));
  dsp.Set("destroy", Napi::Function::New(env, Destroy));
  return dsp;
}

Napi::Value Version(const Napi::CallbackInfo& info) {
  return Napi::String::New(info.Env(), "0.4.0-phase3-basic-filters");
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("version", Napi::Function::New(env, Version));
  exports.Set("createDSP", Napi::Function::New(env, CreateDSP));
  return exports;
}

}  // namespace

NODE_API_MODULE(audio_dsp, Init)

#include <napi.h>
#include "miniaudio.h"
#include <cstdint>
#include <stdexcept>

namespace {

class Decoder final : public Napi::ObjectWrap<Decoder> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function ctor = DefineClass(env, "NativeDecoder", {
            InstanceMethod("read", &Decoder::Read),
            InstanceMethod("seek", &Decoder::Seek),
            InstanceMethod("cursor", &Decoder::Cursor),
            InstanceMethod("length", &Decoder::Length),
            InstanceMethod("sampleRate", &Decoder::SampleRate),
            InstanceMethod("channels", &Decoder::Channels),
            InstanceMethod("destroy", &Decoder::Destroy),
        });
        constructor = Napi::Persistent(ctor);
        constructor.SuppressDestruct();
        exports.Set("NativeDecoder", ctor);
        return exports;
    }

    Decoder(const Napi::CallbackInfo& info)
        : Napi::ObjectWrap<Decoder>(info) {
        if (info.Length() < 1 || !info[0].IsBuffer()) {
            throw Napi::TypeError::New(info.Env(), "NativeDecoder requires an encoded Buffer");
        }

        input_ = Napi::Persistent(info[0].As<Napi::Buffer<uint8_t>>());
        input_.SuppressDestruct();

        ma_decoder_config config = ma_decoder_config_init(
            ma_format_s16,
            2,
            48000);

        const auto result = ma_decoder_init_memory(
            input_.Data(),
            input_.Length(),
            &config,
            &decoder_);
        if (result != MA_SUCCESS) {
            input_.Reset();
            throw Napi::Error::New(info.Env(), "miniaudio could not decode this audio format");
        }
        initialized_ = true;
    }

    ~Decoder() override {
        Uninit();
    }

private:
    static Napi::FunctionReference constructor;

    ma_decoder decoder_{};
    bool initialized_ = false;
    Napi::Reference<Napi::Buffer<uint8_t>> input_;

    void EnsureAlive(const Napi::Env& env) const {
        if (!initialized_) {
            throw Napi::Error::New(env, "NativeDecoder has been destroyed");
        }
    }

    void Uninit() {
        if (initialized_) {
            ma_decoder_uninit(&decoder_);
            initialized_ = false;
        }
        input_.Reset();
    }

    Napi::Value Read(const Napi::CallbackInfo& info) {
        auto env = info.Env();
        EnsureAlive(env);
        if (info.Length() < 1 || !info[0].IsNumber()) {
            throw Napi::TypeError::New(env, "read() requires a frame count");
        }
        const double requested = info[0].As<Napi::Number>().DoubleValue();
        if (!std::isfinite(requested) || requested < 0 || requested > 1'048'576) {
            throw Napi::RangeError::New(env, "invalid frame count");
        }
        const ma_uint64 frames = static_cast<ma_uint64>(requested);
        auto out = Napi::Buffer<uint8_t>::New(env, static_cast<size_t>(frames) * 2u * sizeof(int16_t));
        ma_uint64 framesRead = frames;
        if (frames > 0) {
            const auto result = ma_decoder_read_pcm_frames(&decoder_, out.Data(), frames, &framesRead);
            if (result != MA_SUCCESS && result != MA_AT_END) {
                throw Napi::Error::New(env, "miniaudio failed to decode PCM frames");
            }
        }
        if (framesRead == frames) return out;
        return Napi::Buffer<uint8_t>::Copy(env, out.Data(), static_cast<size_t>(framesRead) * 2u * sizeof(int16_t));
    }

    Napi::Value Seek(const Napi::CallbackInfo& info) {
        auto env = info.Env();
        EnsureAlive(env);
        if (info.Length() < 1 || !info[0].IsNumber()) {
            throw Napi::TypeError::New(env, "seek() requires a PCM frame position");
        }
        const double position = info[0].As<Napi::Number>().DoubleValue();
        if (!std::isfinite(position) || position < 0) {
            throw Napi::RangeError::New(env, "invalid PCM frame position");
        }
        const auto result = ma_decoder_seek_to_pcm_frame(&decoder_, static_cast<ma_uint64>(position));
        if (result != MA_SUCCESS) {
            throw Napi::Error::New(env, "miniaudio failed to seek to the requested PCM frame");
        }
        return env.Undefined();
    }

    Napi::Value Cursor(const Napi::CallbackInfo& info) {
        auto env = info.Env();
        EnsureAlive(env);
        ma_uint64 cursor = 0;
        if (ma_decoder_get_cursor_in_pcm_frames(&decoder_, &cursor) != MA_SUCCESS) {
            throw Napi::Error::New(env, "miniaudio failed to get decoder cursor");
        }
        return Napi::Number::New(env, static_cast<double>(cursor));
    }

    Napi::Value Length(const Napi::CallbackInfo& info) {
        auto env = info.Env();
        EnsureAlive(env);
        ma_uint64 length = 0;
        if (ma_decoder_get_length_in_pcm_frames(&decoder_, &length) != MA_SUCCESS) {
            throw Napi::Error::New(env, "miniaudio failed to get decoder length");
        }
        return Napi::Number::New(env, static_cast<double>(length));
    }

    Napi::Value SampleRate(const Napi::CallbackInfo& info) {
        EnsureAlive(info.Env());
        return Napi::Number::New(info.Env(), static_cast<double>(decoder_.outputSampleRate));
    }

    Napi::Value Channels(const Napi::CallbackInfo& info) {
        EnsureAlive(info.Env());
        return Napi::Number::New(info.Env(), static_cast<double>(decoder_.outputChannels));
    }

    Napi::Value Destroy(const Napi::CallbackInfo& info) {
        Uninit();
        return info.Env().Undefined();
    }
};

Napi::FunctionReference Decoder::constructor;

} // namespace

Napi::Object InitDecoder(Napi::Env env, Napi::Object exports) {
    return Decoder::Init(env, exports);
}

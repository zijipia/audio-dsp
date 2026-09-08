#include <napi.h>
#include "miniaudio.h"
#include <cmath>
#include <cstdint>

namespace
{
    class Decoder final : public Napi::ObjectWrap<Decoder>
    {
    public:
        static Napi::Object Init(Napi::Env env, Napi::Object exports)
        {
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
        Decoder(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Decoder>(info)
        {
            auto env = info.Env();
            if (info.Length() < 1 || (!info[0].IsBuffer() && !info[0].IsString()))
                throw Napi::TypeError::New(env, "NativeDecoder requires an encoded Buffer or file path");
            ma_decoder_config config = ma_decoder_config_init(ma_format_s16, 2, 48000);
            ma_result result = MA_ERROR;
            if (info[0].IsBuffer())
            {
                if (info[0].IsBuffer())
                {
                    auto buffer = info[0].As<Napi::Buffer<uint8_t>>();

                    if (buffer.Length() == 0)
                    {
                        throw Napi::RangeError::New(
                            env,
                            "encoded Buffer must not be empty");
                    }

                    input_ = Napi::Persistent(buffer);

                    result = ma_decoder_init_memory(
                        buffer.Data(),
                        buffer.Length(),
                        &config,
                        &decoder_);
                }
                else
                {
                    const auto path = info[0].As<Napi::String>().Utf8Value();

                    if (path.empty())
                    {
                        throw Napi::RangeError::New(
                            env,
                            "file path must not be empty");
                    }

                    result = ma_decoder_init_file(
                        path.c_str(),
                        &config,
                        &decoder_);
                }
            }
            else
            {
                const auto path = info[0].As<Napi::String>().Utf8Value();
                if (path.empty())
                    throw Napi::RangeError::New(env, "file path must not be empty");
                result = ma_decoder_init_file(path.c_str(), &config, &decoder_);
            }
            if (result != MA_SUCCESS)
            {
                input_.Reset();
                throw Napi::Error::New(env, "miniaudio could not decode this audio source");
            }
            initialized_ = true;
        }
        ~Decoder() override { Uninit(); }

    private:
        static Napi::FunctionReference constructor;
        ma_decoder decoder_{};
        bool initialized_ = false;
        Napi::Reference<Napi::Buffer<uint8_t>> input_;
        void EnsureAlive(const Napi::Env &env) const
        {
            if (!initialized_)
                throw Napi::Error::New(env, "NativeDecoder has been destroyed");
        }
        void Uninit()
        {
            if (initialized_)
            {
                ma_decoder_uninit(&decoder_);
                initialized_ = false;
            }
            input_.Reset();
        }
        Napi::Value Read(const Napi::CallbackInfo &info)
        {
            auto env = info.Env();
            EnsureAlive(env);
            if (info.Length() < 1 || !info[0].IsNumber())
                throw Napi::TypeError::New(env, "read() requires a frame count");
            double requested = info[0].As<Napi::Number>().DoubleValue();
            if (!std::isfinite(requested) || requested < 0 || requested > 1048576)
                throw Napi::RangeError::New(env, "invalid frame count");
            ma_uint64 frames = (ma_uint64)requested;
            auto out = Napi::Buffer<uint8_t>::New(env, (size_t)frames * 2 * sizeof(int16_t));
            ma_uint64 read = frames;
            if (frames)
            {
                auto r = ma_decoder_read_pcm_frames(&decoder_, out.Data(), frames, &read);
                if (r != MA_SUCCESS && r != MA_AT_END)
                    throw Napi::Error::New(env, "miniaudio failed to decode PCM frames");
            }
            return read == frames ? out : Napi::Buffer<uint8_t>::Copy(env, out.Data(), (size_t)read * 2 * sizeof(int16_t));
        }
        Napi::Value Seek(const Napi::CallbackInfo &info)
        {
            auto env = info.Env();
            EnsureAlive(env);
            if (info.Length() < 1 || !info[0].IsNumber())
                throw Napi::TypeError::New(env, "seek() requires a PCM frame position");
            double p = info[0].As<Napi::Number>().DoubleValue();
            if (!std::isfinite(p) || p < 0)
                throw Napi::RangeError::New(env, "invalid PCM frame position");
            if (ma_decoder_seek_to_pcm_frame(&decoder_, (ma_uint64)p) != MA_SUCCESS)
                throw Napi::Error::New(env, "miniaudio failed to seek to the requested PCM frame");
            return env.Undefined();
        }
        Napi::Value Cursor(const Napi::CallbackInfo &info)
        {
            auto env = info.Env();
            EnsureAlive(env);
            ma_uint64 v = 0;
            if (ma_decoder_get_cursor_in_pcm_frames(&decoder_, &v) != MA_SUCCESS)
                throw Napi::Error::New(env, "miniaudio failed to get decoder cursor");
            return Napi::Number::New(env, (double)v);
        }
        Napi::Value Length(const Napi::CallbackInfo &info)
        {
            auto env = info.Env();
            EnsureAlive(env);
            ma_uint64 v = 0;
            if (ma_decoder_get_length_in_pcm_frames(&decoder_, &v) != MA_SUCCESS)
                throw Napi::Error::New(env, "miniaudio failed to get decoder length");
            return Napi::Number::New(env, (double)v);
        }
        Napi::Value SampleRate(const Napi::CallbackInfo &info)
        {
            EnsureAlive(info.Env());
            return Napi::Number::New(info.Env(), (double)decoder_.outputSampleRate);
        }
        Napi::Value Channels(const Napi::CallbackInfo &info)
        {
            EnsureAlive(info.Env());
            return Napi::Number::New(info.Env(), (double)decoder_.outputChannels);
        }
        Napi::Value Destroy(const Napi::CallbackInfo &info)
        {
            Uninit();
            return info.Env().Undefined();
        }
    };
    Napi::FunctionReference Decoder::constructor;
}
Napi::Object InitDecoder(Napi::Env env, Napi::Object exports) { return Decoder::Init(env, exports); }
NODE_API_MODULE(audio_decoder, InitDecoder)

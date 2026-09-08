#include "dsp_data_source.h"

#include <vector>

namespace audio_dsp {
namespace {

static DSPDataSource* self(ma_data_source* p) {
    return reinterpret_cast<DSPDataSource*>(p);
}

static ma_result read_pcm_frames(ma_data_source* pDataSource, void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead) {
    auto* s = self(pDataSource);
    if (pFramesRead) *pFramesRead = 0;
    if (frameCount == 0) return MA_SUCCESS;

    if (pFramesOut == nullptr) {
        ma_uint64 framesRead = 0;
        ma_result r = ma_data_source_read_pcm_frames(s->upstream, nullptr, frameCount, &framesRead);
        if (r != MA_SUCCESS && r != MA_AT_END) return r;
        s->cursor += framesRead;
        if (framesRead > 0 && s->resetState) s->resetState(s->dsp);
        if (pFramesRead) *pFramesRead = framesRead;
        return r;
    }

    std::vector<ma_uint8> input(static_cast<size_t>(frameCount) * s->bytesPerFrame);
    ma_uint64 framesRead = 0;
    ma_result r = ma_data_source_read_pcm_frames(s->upstream, input.data(), frameCount, &framesRead);
    if (r != MA_SUCCESS && r != MA_AT_END) return r;

    if (framesRead > 0) {
        r = s->process(s->dsp, input.data(), pFramesOut, static_cast<size_t>(framesRead));
        if (r != MA_SUCCESS) return r;
        s->cursor += framesRead;
    }

    if (pFramesRead) *pFramesRead = framesRead;
    return (r == MA_AT_END || framesRead < frameCount) ? MA_AT_END : MA_SUCCESS;
}

static ma_result seek_to_pcm_frame(ma_data_source* pDataSource, ma_uint64 frameIndex) {
    auto* s = self(pDataSource);
    ma_result r = ma_data_source_seek_to_pcm_frame(s->upstream, frameIndex);
    if (r != MA_SUCCESS) return r;
    s->cursor = frameIndex;
    if (s->resetState) s->resetState(s->dsp);
    return MA_SUCCESS;
}

static ma_result get_data_format(ma_data_source* pDataSource, ma_format* pFormat, ma_uint32* pChannels, ma_uint32* pSampleRate, ma_channel* pChannelMap, size_t channelMapCap) {
    auto* s = self(pDataSource);
    return ma_data_source_get_data_format(s->upstream, pFormat, pChannels, pSampleRate, pChannelMap, channelMapCap);
}

static ma_result get_cursor_in_pcm_frames(ma_data_source* pDataSource, ma_uint64* pCursor) {
    auto* s = self(pDataSource);
    *pCursor = s->cursor;
    return MA_SUCCESS;
}

static ma_result get_length_in_pcm_frames(ma_data_source* pDataSource, ma_uint64* pLength) {
    auto* s = self(pDataSource);
    return ma_data_source_get_length_in_pcm_frames(s->upstream, pLength);
}

static ma_result set_looping(ma_data_source* pDataSource, ma_bool32 isLooping) {
    auto* s = self(pDataSource);
    return ma_data_source_set_looping(s->upstream, isLooping);
}

static ma_data_source_vtable g_vtable = {
    read_pcm_frames,
    seek_to_pcm_frame,
    get_data_format,
    get_cursor_in_pcm_frames,
    get_length_in_pcm_frames,
    set_looping,
    0
};

} // namespace

ma_result dsp_data_source_init(DSPDataSource* s, ma_data_source* upstream, void* dsp, ProcessFn process, ResetStateFn resetState) {
    if (!s || !upstream || !dsp || !process) return MA_INVALID_ARGS;

    ma_format format = ma_format_unknown;
    ma_uint32 channels = 0;
    ma_uint32 sampleRate = 0;
    ma_result r = ma_data_source_get_data_format(upstream, &format, &channels, &sampleRate, nullptr, 0);
    if (r != MA_SUCCESS) return r;
    if (format == ma_format_unknown || channels == 0) return MA_INVALID_DATA;

    ma_uint32 bps = ma_get_bytes_per_sample(format);
    if (bps == 0) return MA_INVALID_DATA;

    ma_data_source_config config = ma_data_source_config_init();
    config.vtable = &g_vtable;
    r = ma_data_source_init(&config, &s->base);
    if (r != MA_SUCCESS) return r;

    s->upstream = upstream;
    s->dsp = dsp;
    s->process = process;
    s->resetState = resetState;
    s->format = format;
    s->channels = channels;
    s->bytesPerFrame = bps * channels;
    s->cursor = 0;
    return MA_SUCCESS;
}

void dsp_data_source_uninit(DSPDataSource* s) {
    if (!s) return;
    ma_data_source_uninit(&s->base);
    s->upstream = nullptr;
    s->dsp = nullptr;
    s->process = nullptr;
    s->resetState = nullptr;
    s->format = ma_format_unknown;
    s->channels = 0;
    s->bytesPerFrame = 0;
    s->cursor = 0;
}

ma_data_source* dsp_data_source_get(DSPDataSource* s) {
    return s ? &s->base : nullptr;
}

} // namespace audio_dsp

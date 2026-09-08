#pragma once

#include "miniaudio.h"
#include <cstddef>
#include <vector>

namespace audio_dsp {

using ProcessFn = ma_result (*)(void* dsp, const void* input, void* output, size_t frameCount);
using ResetStateFn = void (*)(void* dsp);

struct DSPDataSource {
    ma_data_source_base base{};
    ma_data_source* upstream = nullptr;
    void* dsp = nullptr;
    ProcessFn process = nullptr;
    ResetStateFn resetState = nullptr;
    ma_format format = ma_format_unknown;
    ma_uint32 channels = 0;
    ma_uint32 bytesPerFrame = 0;
    ma_uint64 cursor = 0;
    std::vector<ma_uint8> scratch;
};

ma_result dsp_data_source_init(
    DSPDataSource* self,
    ma_data_source* upstream,
    void* dsp,
    ProcessFn process,
    ResetStateFn resetState);

void dsp_data_source_uninit(DSPDataSource* self);

ma_data_source* dsp_data_source_get(DSPDataSource* self);

} // namespace audio_dsp

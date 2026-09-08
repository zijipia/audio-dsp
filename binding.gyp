{
  "targets": [
    {
      "target_name": "audio_dsp",
      "sources": [
        "native/audio_dsp.cc",
        "native/dsp_data_source.cc",
        "native/miniaudio_impl.cc"
      ],
      "include_dirs": ["<!@(node -p \"require('node-addon-api').include\")", "third_party/miniaudio"],
      "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"],
      "defines": ["NODE_ADDON_API_CPP_EXCEPTIONS"],
      "cflags_cc": ["-fexceptions"]
    },
    {
      "target_name": "audio_decoder",
      "sources": ["native/audio_decoder.cc", "native/miniaudio_impl.cc"],
      "include_dirs": ["<!@(node -p \"require('node-addon-api').include\")", "third_party/miniaudio"],
      "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"],
      "defines": ["NODE_ADDON_API_CPP_EXCEPTIONS"],
      "cflags_cc": ["-fexceptions"]
    }
  ]
}

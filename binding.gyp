{
  "targets": [
    {
      "target_name": "audio_dsp",
      "sources": [
        "native/audio_dsp.cc",
        "native/dsp_data_source.cc",
        "native/miniaudio_impl.cc"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "third_party/miniaudio"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "defines": [
        "NODE_ADDON_API_CPP_EXCEPTIONS"
      ],
      "cflags_cc": [
        "-fexceptions"
      ],
      "conditions": [
        ["OS==\"win\"", {
          "defines": ["NOMINMAX"],
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1
            }
          }
        }],
        ["OS==\"mac\"", {
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LANGUAGE_STANDARD": "c++17"
          }
        }]
      ]
    }
  ]
}

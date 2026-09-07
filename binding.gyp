{
  "targets": [
    {
      "target_name": "audio_dsp",
      "sources": ["native/audio_dsp.cc"],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS"],
      "cflags_cc!": ["-fno-exceptions"],
      "conditions": [
        ["OS==\"win\"", {
          "defines": ["NOMINMAX"]
        }]
      ]
    }
  ]
}

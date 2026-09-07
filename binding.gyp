{
  "targets": [
    {
      "target_name": "audio_dsp",
      "sources": [
        "native/audio_dsp.cc",
        "native/miniaudio_impl.cc"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "third_party/miniaudio"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "conditions": [
        ["OS==\"win\"", {
          "defines": ["NOMINMAX"]
        }]
      ]
    }
  ]
}

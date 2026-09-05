{
  "targets": [
    {
      "target_name": "node-librados",
      "sources": [
        "src/addon.cpp",
        "src/cluster.cpp",
        "src/events.cpp",
        "src/io_context.cpp",
        "src/rbd.cpp"
      ],
      "include_dirs": ["<!@(node -p \"require('node-addon-api').include\")"],
      "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"],
      "libraries": ["-lrados", "-lrbd", "-ldl"],
      "defines": ["NAPI_VERSION=9"],
      "cflags_cc": ["-std=c++17", "-Wall", "-Wextra", "-Wpedantic"],
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"]
    }
  ]
}

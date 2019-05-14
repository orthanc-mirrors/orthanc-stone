#pragma once

#if ORTHANC_ENABLE_WASM == 1
// https://emscripten.org/docs/optimizing/Optimizing-WebGL.html
#  define ORTHANC_STONE_OPENGL_SHADER_VERSION_DIRECTIVE "precision mediump float;\n"
#else
#  define ORTHANC_STONE_OPENGL_SHADER_VERSION_DIRECTIVE "#version 110\n"
#endif

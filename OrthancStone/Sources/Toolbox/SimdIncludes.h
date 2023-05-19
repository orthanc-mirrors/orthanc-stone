/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2022 Osimis S.A., Belgium
 * Copyright (C) 2021-2022 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#pragma once

#if defined(__EMSCRIPTEN__)
#  include <emscripten.h>
#  include <wasm_simd128.h>
#else
#  include <immintrin.h>  // portable to all x86 compilers
#endif

#if __AVX2__ == 1
#  define ORTHANC_HAS_AVX2          1
#  define ORTHANC_HAS_SSE2          1
#  define ORTHANC_HAS_WASM_SIMD     0
#  define ORTHANC_MEMORY_ALIGNMENT  32
#elif __SSE2__ == 1
#  define ORTHANC_HAS_AVX2          0
#  define ORTHANC_HAS_SSE2          1
#  define ORTHANC_HAS_WASM_SIMD     0
#  define ORTHANC_MEMORY_ALIGNMENT  16
#elif defined(__EMSCRIPTEN__)
#  if !defined(ORTHANC_HAS_WASM_SIMD)
#    error ORTHANC_HAS_WASM_SIMD must be defined to use this file
#  endif
#  define ORTHANC_HAS_AVX2          0
#  define ORTHANC_HAS_SSE2          0
#  if ORTHANC_HAS_WASM_SIMD == 1
//   Setting macro "ORTHANC_HAS_WASM_SIMD" to "1" means that
//   "-msimd128" has been provided to Emscripten (there doesn't seem
//   to exist a predefined macro to automatically check this)
#    define ORTHANC_MEMORY_ALIGNMENT  16
#  else
#    define ORTHANC_MEMORY_ALIGNMENT  8
#  endif
#elif defined(_MSC_VER)
#  if _M_IX86_FP >= 2   // https://stackoverflow.com/a/18563988
#    define ORTHANC_HAS_AVX2          0
#    define ORTHANC_HAS_SSE2          0
#    define ORTHANC_HAS_WASM_SIMD     1
#    define ORTHANC_MEMORY_ALIGNMENT  16
#  endif
#else
#  define ORTHANC_MEMORY_ALIGNMENT  8
#endif

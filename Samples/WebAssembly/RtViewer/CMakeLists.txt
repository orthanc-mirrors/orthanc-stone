cmake_minimum_required(VERSION 2.8.3)

project(RtViewerWasm)

# Configuration of the Emscripten compiler for WebAssembly target
# ---------------------------------------------------------------
set(USE_WASM ON CACHE BOOL "")
set(ORTHANC_FRAMEWORK_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../../orthanc CACHE STRING "")
set(STONE_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../)

set(EMSCRIPTEN_SET_LLVM_WASM_BACKEND ON CACHE BOOL "")

set(WASM_FLAGS "-s WASM=1 -s FETCH=1")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(WASM_FLAGS "${WASM_FLAGS} -s SAFE_HEAP=1")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WASM_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WASM_FLAGS}")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXTRA_EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\"]'")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ERROR_ON_UNDEFINED_SYMBOLS=1")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ASSERTIONS=1 -s DISABLE_EXCEPTION_CATCHING=0")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=268435456")  # 256MB + resize
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1")
add_definitions(
  -DDISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1
)

# Stone of Orthanc configuration
# ---------------------------------------------------------------
set(ALLOW_DOWNLOADS ON)
set(ORTHANC_FRAMEWORK_SOURCE "path")

include(${STONE_ROOT}/Resources/CMake/OrthancStoneParameters.cmake)

SET(ENABLE_DCMTK ON)
SET(ENABLE_GOOGLE_TEST OFF)
SET(ENABLE_LOCALE ON)  # Necessary for text rendering
SET(ENABLE_WASM ON)
SET(ORTHANC_SANDBOXED ON)

# this will set up the build system for Stone of Orthanc and will
# populate the ORTHANC_STONE_SOURCES CMake variable
include(${STONE_ROOT}/Resources/CMake/OrthancStoneConfiguration.cmake)

include_directories(${STONE_ROOT})

# Define the WASM module
# ---------------------------------------------------------------
add_executable(RtViewerWasm
  RtViewerWasm.cpp
  ../../Common/RtViewer.cpp
  ../../Common/RtViewer.h

  ${ORTHANC_STONE_SOURCES}
  )

# Declare installation files for the module
# ---------------------------------------------------------------
install(
  TARGETS RtViewerWasm
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
  )

# Declare installation files for the companion files (web scaffolding)
# please note that ${CMAKE_CURRENT_BINARY_DIR}/RtViewerWasm.js
# (the generated JS loader for the WASM module) is handled by the `install1`
# section above: it is considered to be the binary output of 
# the linker.
# ---------------------------------------------------------------
install(
  FILES
  ${CMAKE_SOURCE_DIR}/RtViewerApp.js
  ${CMAKE_SOURCE_DIR}/index.html
  ${CMAKE_CURRENT_BINARY_DIR}/RtViewerWasm.wasm
  ${CMAKE_SOURCE_DIR}/WasmWrapper.js
  DESTINATION ${CMAKE_INSTALL_PREFIX}
  )

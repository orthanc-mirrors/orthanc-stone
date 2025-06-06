# Stone of Orthanc
# Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
# Department, University Hospital of Liege, Belgium
# Copyright (C) 2017-2023 Osimis S.A., Belgium
# Copyright (C) 2021-2025 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
#
# This program is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License
# as published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.


cmake_minimum_required(VERSION 2.8.3...4.0)

project(OrthancStone)

set(ORTHANC_STONE_MODULE_PREFIX "${CMAKE_SOURCE_DIR}/../../../../wasm-binaries/OrthancStoneModule" CACHE PATH "Where to find the precompiled WebAssembly module of Orthanc Stone")
set(ORTHANC_STONE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/../../../../wasm-binaries/StoneWebViewer" CACHE PATH "Where to put the WebAssembly binaries")
set(EMSCRIPTEN_TARGET_MODE "wasm" CACHE STRING "Sets the target mode for Emscripten (can be \"wasm\" or \"asm.js\")")
mark_as_advanced(EMSCRIPTEN_TARGET_MODE)


set(WASM_FLAGS "-s MAIN_MODULE=1")  # Must be before "Compiler.cmake"

# https://github.com/emscripten-core/emscripten/issues/11407
set(WASM_FLAGS "${WASM_FLAGS} -s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE=[\"_emscripten_fetch_xhr\",\"_emscripten_fetch_load_cached_data\",\"_emscripten_fetch_delete_cached_data\",\"_emscripten_fetch_cache_data\"]")

set(WASM_LINKER_FLAGS "${WASM_LINKER_FLAGS} -s RUNTIME_LINKED_LIBS='[\"OrthancStoneModule.wasm\"]'")



# Configuration of the Emscripten compiler for WebAssembly target
# ---------------------------------------------------------------
set(USE_WASM ON CACHE BOOL "")

set(WASM_FLAGS "${WASM_FLAGS} -s WASM=1 -s FETCH=1")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(WASM_FLAGS "${WASM_FLAGS} -s SAFE_HEAP=1")
endif()

set(WASM_LINKER_FLAGS "${WASM_LINKER_FLAGS} -s EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\"]'")
set(WASM_LINKER_FLAGS "${WASM_LINKER_FLAGS} -s ERROR_ON_UNDEFINED_SYMBOLS=1")
set(WASM_LINKER_FLAGS "${WASM_LINKER_FLAGS} -s ASSERTIONS=1 -s DISABLE_EXCEPTION_CATCHING=0")
set(WASM_LINKER_FLAGS "${WASM_LINKER_FLAGS} -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=268435456")  # 256MB + resize
set(WASM_LINKER_FLAGS "${WASM_LINKER_FLAGS} -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1")
add_definitions(
  -DDISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1
)



# Stone of Orthanc configuration, to be used as module
# ---------------------------------------------------------------


include(${CMAKE_SOURCE_DIR}/../../../../OrthancStone/Resources/Orthanc/CMake/Compiler.cmake)
include(${CMAKE_SOURCE_DIR}/../../../../OrthancStone/Resources/Orthanc/CMake/DownloadPackage.cmake)

add_definitions(
  -DORTHANC_STONE_MAX_TAG_LENGTH=256  # TODO => share with OrthancStone
  )

include_directories(
  ${ORTHANC_STONE_MODULE_PREFIX}/include
  ${ORTHANC_STONE_MODULE_PREFIX}/include/cairo
  ${ORTHANC_STONE_MODULE_PREFIX}/include/orthanc-framework
  ${ORTHANC_STONE_MODULE_PREFIX}/include/orthanc-stone
  )



################################################################################


# Create the wrapper to call C++ from JavaScript
# ---------------------------------------------------------------

set(LIBCLANG "libclang-4.0.so.1" CACHE PATH "Version of clang to generate the code model")
set(STONE_WRAPPER ${CMAKE_CURRENT_BINARY_DIR}/stone.js)

add_custom_command(
  COMMAND
  ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/../ParseWebAssemblyExports.py --libclang=${LIBCLANG} ${CMAKE_SOURCE_DIR}/../StoneWebViewer.cpp > ${STONE_WRAPPER}
  DEPENDS
  ${CMAKE_SOURCE_DIR}/../StoneWebViewer.cpp
  ${CMAKE_SOURCE_DIR}/../ParseWebAssemblyExports.py
  OUTPUT
  ${STONE_WRAPPER}
  )

add_custom_target(StoneWrapper
  DEPENDS
  ${STONE_WRAPPER}
  )  


# Define the WASM module
# ---------------------------------------------------------------

aux_source_directory(
  ${ORTHANC_STONE_MODULE_PREFIX}/src/orthanc-stone/
  ORTHANC_STONE_SOURCES)

add_executable(StoneWebViewer
  ${AUTOGENERATED_SOURCES}
  ${ORTHANC_STONE_SOURCES}
  ../StoneWebViewer.cpp
  )

set_target_properties(StoneWebViewer
  PROPERTIES
  COMPILE_FLAGS "${WASM_FLAGS}"
  LINK_FLAGS "${WASM_LINKER_FLAGS}"
  )

# Boost::Math begs for -std=c++14 since Boost 1.82.0
set_property(TARGET StoneWebViewer PROPERTY CXX_STANDARD 14)

# Make sure to have the wrapper generated
add_dependencies(StoneWebViewer StoneWrapper)


# Declare installation files for the module
# ---------------------------------------------------------------

install(
  TARGETS StoneWebViewer
  RUNTIME DESTINATION ${ORTHANC_STONE_INSTALL_PREFIX}
  )


# Declare installation files for the companion files (web scaffolding)
# please note that ${CMAKE_CURRENT_BINARY_DIR}/StoneWebViewer.js
# (the generated JS loader for the WASM module) is handled by the `install`
# section above: it is considered to be the binary output of 
# the linker.
# ---------------------------------------------------------------
install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/StoneWebViewer.wasm
  ${CMAKE_SOURCE_DIR}/../../WebApplication/app.css
  ${CMAKE_SOURCE_DIR}/../../WebApplication/app.js
  ${CMAKE_SOURCE_DIR}/../../WebApplication/index.html
  ${ORTHANC_STONE_MODULE_PREFIX}/lib/OrthancStoneModule.wasm  # NEW
  ${STONE_WRAPPER}
  DESTINATION ${ORTHANC_STONE_INSTALL_PREFIX}
  )

install(
  FILES
  ${CMAKE_SOURCE_DIR}/../../WebApplication/img/grid1x1.png
  ${CMAKE_SOURCE_DIR}/../../WebApplication/img/grid1x2.png
  ${CMAKE_SOURCE_DIR}/../../WebApplication/img/grid2x1.png
  ${CMAKE_SOURCE_DIR}/../../WebApplication/img/grid2x2.png
  ${CMAKE_SOURCE_DIR}/../../WebApplication/img/loading.gif
  DESTINATION ${ORTHANC_STONE_INSTALL_PREFIX}/img
  )

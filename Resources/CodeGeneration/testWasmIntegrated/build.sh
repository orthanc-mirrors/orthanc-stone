#!/bin/bash

set -e

mkdir -p build-wasm
cd build-wasm

# shellcheck source="$HOME/apps/emsdk/emsdk_env.sh"
source "$HOME/apps/emsdk/emsdk_env.sh"
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=${EMSCRIPTEN}/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_WASM=ON ..

ninja

cd ..

./build-web.sh


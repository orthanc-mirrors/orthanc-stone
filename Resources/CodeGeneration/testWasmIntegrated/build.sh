#!/bin/bash

set -e

mkdir -p build-wasm
cd build-wasm

# shellcheck source="$HOME/apps/emsdk/emsdk_env.sh"
source "$HOME/apps/emsdk/emsdk_env.sh"
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=${EMSCRIPTEN}/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_WASM=ON ..

ninja

echo 

cd ..

mkdir -p 

# compile TS to JS
tsc --module commonjs --sourceMap -t ES2015 --outDir "build-tsc/" build-wasm/testWasmIntegratedCpp_generated.ts testWasmIntegrated.ts 

# bundle all JS files to final build dir 
browserify "build-wasm/testWasmIntegratedCpp.js" "build-tsc/testWasmIntegratedCpp_generated.js" "build-tsc/testWasmIntegrated.js" -o "testWasmIntegratedApp.js"

# copy HTML start page to output dir
cp index.html build-final/

# copy WASM binary to output dir
cp build-wasm/testWasmIntegratedCpp.wasm  build-final/




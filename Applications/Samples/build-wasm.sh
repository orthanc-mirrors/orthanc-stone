#!/bin/bash
#
# usage:
# to build all targets:
# ./build-wasm.sh
#
# to build a single target:
# ./build-wasm.sh OrthancStoneSingleFrameEditor

set -e

target=${1:-all}

currentDir=$(pwd)
samplesRootDir=$(pwd)

mkdir -p $samplesRootDir/build-wasm
cd $samplesRootDir/build-wasm

source ~/apps/emsdk/emsdk_env.sh
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=${EMSCRIPTEN}/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_BUILD_TYPE=Release -DSTONE_SOURCES_DIR=$currentDir/../../../orthanc-stone -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT=$currentDir/../../../orthanc -DALLOW_DOWNLOADS=ON .. -DENABLE_WASM=ON
ninja $target

echo "-- building the web application -- "
cd $currentDir
./build-web.sh
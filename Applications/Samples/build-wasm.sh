#!/bin/bash

set -e

currentDir=$(pwd)
samplesRootDir=$(pwd)

mkdir -p $samplesRootDir/build-wasm
cd $samplesRootDir/build-wasm

source ~/Downloads/emsdk/emsdk_env.sh
cmake -DCMAKE_TOOLCHAIN_FILE=${EMSCRIPTEN}/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_BUILD_TYPE=Release -DSTONE_SOURCES_DIR=$currentDir/../../../orthanc-stone -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT=$currentDir/../../../orthanc -DALLOW_DOWNLOADS=ON .. -DENABLE_WASM=ON
make -j 5

echo "-- building the web application -- "
cd $currentDir
./build-web.sh
#!/bin/bash
#
# usage:
# build-wasm BUILD_TYPE
# where BUILD_TYPE is Debug, RelWithDebInfo or Release

set -e

buildType=${1:-Debug}

currentDir=$(pwd)
currentDirAbs=$(realpath $currentDir)

mkdir -p build-wasm
cd build-wasm

source ~/apps/emsdk/emsdk_env.sh
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=${EMSCRIPTEN}/cmake/Modules/Platform/Emscripten.cmake \
-DCMAKE_BUILD_TYPE=$buildType -DSTONE_SOURCES_DIR=$currentDirAbs/../../../../orthanc-stone \
-DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT=$currentDirAbs/../../../../orthanc \
-DALLOW_DOWNLOADS=ON .. -DENABLE_WASM=ON

ninja $target

echo "-- building the web application -- "
cd $currentDir
./build-web.sh

echo "Launch start-serving-files.sh to access the web sample application locally"

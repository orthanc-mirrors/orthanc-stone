#!/bin/bash

currentDir=$(pwd)
wasmRootDir=$(pwd)

mkdir -p $wasmRootDir/build
cd $wasmRootDir/build

source ~/Downloads/emsdk/emsdk_env.sh
cmake -DCMAKE_TOOLCHAIN_FILE=${EMSCRIPTEN}/cmake/Modules/Platform/Emscripten.cmake .. -DCMAKE_BUILD_TYPE=Release -DSTONE_SOURCES_DIR=$currentDir/../../../orthanc-stone -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT=$currentDir/../../../orthanc -DALLOW_DOWNLOADS=ON -DSTATIC_BUILD=OFF
make -j 5

echo "-- building the web application -- "
cd $currentDir
#./build-web.sh
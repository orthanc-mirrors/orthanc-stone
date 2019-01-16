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

# we will create the output files in a "build-wasm" folder in current location

# let's save current dir
currentDir=$(pwd)

scriptDirRel=$(dirname $0)
#echo $scriptDirRel
scriptDirAbs=$(realpath $scriptDirRel)
echo $scriptDirAbs

pushd
# echo "$0"
# echo $BASH_SOURCE
# echo "$BASH_SOURCE"
# scriptDir=dirname $0
# echo "***********"
# echo "***********"

#echo "Script folder is" 
#echo "$(cd "$(dirname "$BASH_SOURCE")"; pwd)/$(basename ""$BASH_SOURCE")"

#samplesRootDir=$(pwd)
samplesRootDir=${scriptDirAbs}
echo "samplesRootDir = " ${samplesRootDir}

mkdir -p build-wasm
cd build-wasm

source ~/apps/emsdk/emsdk_env.sh
cmake -DCMAKE_TOOLCHAIN_FILE=${EMSCRIPTEN}/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_BUILD_TYPE=Release -DSTONE_SOURCES_DIR=$samplesRootDir/../.. -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT=$samplesRootDir/../../../orthanc -DALLOW_DOWNLOADS=ON $samplesRootDir -DENABLE_WASM=ON
make -j 5 $target

echo "-- building the web application -- "
cd ${samplesRootDir}
./build-web-ext.sh

popd

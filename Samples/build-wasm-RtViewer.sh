#!/bin/bash
#
# usage:
# to build the sample in Debug:
# ./build-wasm-RtViewer.sh
#
# to build the sample in Debug:
# ./build-wasm-RtViewer.sh Release

set -e

if [ ! -d "WebAssembly" ]; then
  echo "This script must be run from the Samples folder one level below orthanc-stone"
  exit 1
fi


currentDir=$(pwd)
samplesRootDir=$(pwd)
devrootDir=$(pwd)/../../

buildType=${1:-RelWithDebInfo}
buildFolderName="$devrootDir/out/build-stone-wasm-RtViewer-$buildType"
installFolderName="$devrootDir/out/install-stone-wasm-RtViewer-$buildType"

mkdir -p $buildFolderName
# change current folder to the build folder
pushd $buildFolderName

# configure the environment to use Emscripten
source ~/apps/emsdk/emsdk_env.sh

emcmake cmake -G "Ninja" \
  -DCMAKE_BUILD_TYPE=$buildType \
  -DCMAKE_INSTALL_PREFIX=$installFolderName \
  -DSTATIC_BUILD=ON -DALLOW_DOWNLOADS=ON \
  $samplesRootDir/WebAssembly/RtViewer

# perform build + installation
ninja install

# restore the original working folder
popd

echo "If all went well, the output files can be found in $installFolderName:"

ls $installFolderName

#!/bin/bash
set -ex

source /opt/emsdk/emsdk_env.sh

# Use a folder that is writeable by non-root users for the Emscripten cache
export EM_CACHE=/tmp/emscripten-cache

# Get the Orthanc framework
cd /tmp/
hg clone https://hg.orthanc-server.com/orthanc/

# Make a copy of the read-only folder containing the source code into
# a writeable folder, because of "DownloadPackage.cmake" that writes
# to the "ThirdPartyDownloads" folder next to the "CMakeLists.txt"
cd /source
hg clone /source /tmp/source-writeable

mkdir /tmp/build
cd /tmp/build

cmake /tmp/source-writeable/OrthancStone/Samples/WebAssembly \
      -DCMAKE_BUILD_TYPE=$1 \
      -DCMAKE_INSTALL_PREFIX=/target \
      -DCMAKE_TOOLCHAIN_FILE=${EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
      -DORTHANC_FRAMEWORK_ROOT=/tmp/orthanc/OrthancFramework/Sources \
      -DSTATIC_BUILD=ON \
      -G Ninja

ninja -j2 install

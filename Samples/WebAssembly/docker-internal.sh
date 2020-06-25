#!/bin/bash
set -ex

source /opt/emsdk/emsdk_env.sh

# Use a folder that is writeable by non-root users for the Emscripten cache
export EM_CACHE=/tmp/emscripten-cache

# Get the Orthanc framework
cd /tmp/
hg clone https://hg.orthanc-server.com/orthanc/

# Make a copy of the source code in a writeable folder, because of
# "DownloadPackage.cmake" that writes to the "ThirdPartyDownloads"
# folder next to the "CMakeLists.txt". We don't use "hg archive", but
# "hg clone", as the "/source" folder might be created by a more
# recent version of Mercurial, triggering the error "repository
# requires features unknown to this Mercurial: sparserevlog!"
# (cf. https://www.mercurial-scm.org/wiki/MissingRequirement).
cd /source
hg clone /source /tmp/source-writeable

mkdir /tmp/build
cd /tmp/build

cmake /tmp/source-writeable/Samples/WebAssembly \
      -DCMAKE_BUILD_TYPE=$1 \
      -DCMAKE_INSTALL_PREFIX=/target \
      -DCMAKE_TOOLCHAIN_FILE=${EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
      -DORTHANC_FRAMEWORK_ROOT=/tmp/orthanc \
      -DSTATIC_BUILD=ON \
      -G Ninja

ninja -j2 install

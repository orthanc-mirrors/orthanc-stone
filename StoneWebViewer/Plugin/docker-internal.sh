#!/bin/bash
set -ex

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

LSB_CC=gcc-4.8 LSB_CXX=g++-4.8 cmake /tmp/source-writeable/StoneWebViewer/Plugin \
      -DCMAKE_BUILD_TYPE=$1 \
      -DCMAKE_INSTALL_PREFIX=/target/ \
      -DCMAKE_TOOLCHAIN_FILE=/tmp/orthanc/OrthancFramework/Resources/Toolchains/LinuxStandardBaseToolchain.cmake \
      -DORTHANC_FRAMEWORK_ROOT=/tmp/orthanc/OrthancFramework \
      -DSTATIC_BUILD=ON \
      -DSTONE_BINARIES_WEB_VIEWER=/target/StoneWebViewer/ \
      -DSTONE_BINARIES_RT_VIEWER=/target/RtViewer/ \
      -DUSE_LEGACY_JSONCPP=ON \
      -G Ninja

ninja -j2 install


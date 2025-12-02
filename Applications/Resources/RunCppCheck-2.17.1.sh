#!/bin/bash
# Note: This script is tuned to run with cppcheck v2.17.1

set -ex

CPPCHECK=cppcheck

if [ $# -ge 1 ]; then
    CPPCHECK=$1
fi

cat <<EOF > /tmp/cppcheck-suppressions.txt
assertWithSideEffect:../../OrthancStone/Sources/Loaders/OrthancMultiframeVolumeLoader.cpp:341
assertWithSideEffect:../../OrthancStone/Sources/Loaders/OrthancMultiframeVolumeLoader.cpp:342
constParameterPointer:../../OrthancStone/Sources/Platforms/WebAssembly/WebAssemblyOracle.cpp:59
constParameterPointer:../../RenderingPlugin/Resources/Orthanc/Plugins/OrthancPluginCppWrapper.h:1020
constVariableReference:../../OrthancStone/Sources/Scene2D/GrayscaleWindowingSceneTracker.cpp:53
knownConditionTrueFalse:../../Applications/StoneWebViewer/Plugin/Plugin.cpp:99
knownConditionTrueFalse:../../OrthancStone/Sources/Toolbox/ImageGeometry.cpp:184
stlFindInsert:../../Applications/StoneWebViewer/WebAssembly/StoneWebViewer.cpp:1813
unusedStructMember:../../OrthancStone/Sources/Platforms/WebAssembly/WebAssemblyOracle.cpp:293
EOF

CPPCHECK_BUILD_DIR=/tmp/cppcheck-build-dir-stone-2.7.1/
mkdir -p ${CPPCHECK_BUILD_DIR}

${CPPCHECK} -j8 --enable=all --quiet --std=c++03 \
            --cppcheck-build-dir=${CPPCHECK_BUILD_DIR} \
            --suppress=unusedFunction \
            --suppress=missingIncludeSystem \
            --suppress=useStlAlgorithm \
            --check-level=exhaustive \
            --suppressions-list=/tmp/cppcheck-suppressions.txt \
            -DHAS_ORTHANC_EXCEPTION=1 \
            -DORTHANC_BUILDING_FRAMEWORK_LIBRARY=1 \
            -DORTHANC_ENABLE_BASE64=1 \
            -DORTHANC_ENABLE_CIVETWEB=0 \
            -DORTHANC_ENABLE_CURL=1 \
            -DORTHANC_ENABLE_DCMTK=1 \
            -DORTHANC_ENABLE_DCMTK_JPEG=1 \
            -DORTHANC_ENABLE_DCMTK_JPEG_LOSSLESS=1 \
            -DORTHANC_ENABLE_GLEW=1 \
            -DORTHANC_ENABLE_JPEG=1 \
            -DORTHANC_ENABLE_LOCALE=1 \
            -DORTHANC_ENABLE_LOGGING=1 \
            -DORTHANC_ENABLE_LOGGING_STDIO=1 \
            -DORTHANC_ENABLE_MD5=1 \
            -DORTHANC_ENABLE_MONGOOSE=0 \
            -DORTHANC_ENABLE_OPENGL=1 \
            -DORTHANC_ENABLE_PKCS11=0 \
            -DORTHANC_ENABLE_PNG=1 \
            -DORTHANC_ENABLE_PUGIXML=1 \
            -DORTHANC_ENABLE_SDL=1 \
            -DORTHANC_ENABLE_SSL=1 \
            -DORTHANC_ENABLE_THREADS=1 \
            -DORTHANC_ENABLE_WASM=1 \
            -DORTHANC_ENABLE_ZLIB=1 \
            -DORTHANC_SANDBOXED=0 \
            -D__GNUC__ \
            -D__cplusplus=201103 \
            -D__linux__ \
            -DEM_ASM\(...\) \
            -UNDEBUG \
            -I${HOME}/Subversion/orthanc/OrthancFramework/Sources \
            -I${HOME}/Subversion/orthanc/OrthancServer/Plugins/Include/ \
            \
            ../../Applications/Samples \
            ../../Applications/StoneWebViewer \
            ../../OrthancStone/Sources \
            ../../OrthancStone/UnitTestsSources \
            ../../RenderingPlugin/Sources \
            \
            -i ../../OrthancStone/UnitTestsSources/SortedFramesTests.cpp
            \
            2>&1

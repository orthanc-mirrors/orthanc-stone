#!/bin/bash

set -e

target=${1:-all}
# this script currently assumes that the wasm code has been built on its side and is availabie in build-wasm/

currentDir=$(pwd)
samplesRootDir=$(pwd)

tscOutput=$samplesRootDir/build-tsc-output/
outputDir=$samplesRootDir/build-web/
mkdir -p "$outputDir"

# files used by all single files samples
cp "$samplesRootDir/index.html" "$outputDir"
cp "$samplesRootDir/samples-styles.css" "$outputDir"

# build rt-viewer-demo
cp $samplesRootDir/rt-viewer-demo.html $outputDir
tsc --project $samplesRootDir/rt-viewer-demo.tsconfig.json --outDir "$tscOutput"
browserify \
    "$tscOutput/orthanc-stone/Platforms/Wasm/logger.js" \
    "$tscOutput/orthanc-stone/Platforms/Wasm/stone-framework-loader.js" \
    "$tscOutput/orthanc-stone/Platforms/Wasm/wasm-application-runner.js" \
    "$tscOutput/orthanc-stone/Platforms/Wasm/wasm-viewport.js" \
    "$tscOutput/rt-viewer-sample/rt-viewer-demo.js" \
    -o "$outputDir/app-rt-viewer-demo.js"
cp "$currentDir/build-wasm/RtViewerDemo.js"  $outputDir
cp "$currentDir/build-wasm/RtViewerDemo.wasm"  $outputDir

cd $currentDir

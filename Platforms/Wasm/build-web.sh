#!/bin/bash

# this script currently assumes that the wasm code has been built on its side and is availabie in Wasm/build/

currentDir=$(pwd)
wasmRootDir=$(pwd)
samplesRootDir=$(pwd)/../../Applications/Samples/

outputDir=$wasmRootDir/build-web/
mkdir -p $outputDir

cp $samplesRootDir/Web/index.html $outputDir
cp $samplesRootDir/Web/samples-styles.css $outputDir

cp $samplesRootDir/Web/simple-viewer.html $outputDir
tsc --allowJs --project $samplesRootDir/Web/tsconfig-simple-viewer.json
cp $currentDir/build/OrthancStoneSimpleViewer.js  $outputDir
cp $currentDir/build/OrthancStoneSimpleViewer.wasm  $outputDir


# cat ../wasm/build/wasm-app.js $currentDir/../../../orthanc-stone/Platforms/WebAssembly/defaults.js > $outputDir/app.js

cd $currentDir


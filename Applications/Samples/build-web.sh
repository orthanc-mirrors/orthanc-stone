#!/bin/bash

set -e

# this script currently assumes that the wasm code has been built on its side and is availabie in Wasm/build/

currentDir=$(pwd)
samplesRootDir=$(pwd)

outputDir=$samplesRootDir/build-web/
mkdir -p $outputDir

cp $samplesRootDir/Web/index.html $outputDir
cp $samplesRootDir/Web/samples-styles.css $outputDir

cp $samplesRootDir/Web/simple-viewer.html $outputDir
tsc --allowJs --project $samplesRootDir/Web/tsconfig-simple-viewer.json
cp $currentDir/build-wasm/OrthancStoneSimpleViewer.js  $outputDir
cp $currentDir/build-wasm/OrthancStoneSimpleViewer.wasm  $outputDir

cd $currentDir

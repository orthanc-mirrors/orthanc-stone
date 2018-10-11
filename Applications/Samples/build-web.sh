#!/bin/bash

set -e

# this script currently assumes that the wasm code has been built on its side and is availabie in build-wasm/

currentDir=$(pwd)
samplesRootDir=$(pwd)

outputDir=$samplesRootDir/build-web/
mkdir -p $outputDir

# files used by all single files samples
cp $samplesRootDir/Web/index.html $outputDir
cp $samplesRootDir/Web/samples-styles.css $outputDir

# build simple-viewer-single-file (obsolete project)
cp $samplesRootDir/Web/simple-viewer-single-file.html $outputDir
tsc --allowJs --project $samplesRootDir/Web/tsconfig-simple-viewer-single-file.json
cp $currentDir/build-wasm/OrthancStoneSimpleViewerSingleFile.js  $outputDir
cp $currentDir/build-wasm/OrthancStoneSimpleViewerSingleFile.wasm  $outputDir

# build simple-viewer project
mkdir -p $outputDir/simple-viewer/
cp $samplesRootDir/SimpleViewer/Wasm/simple-viewer.html $outputDir/simple-viewer/
cp $samplesRootDir/SimpleViewer/Wasm/styles.css $outputDir/simple-viewer/
tsc --allowJs --project $samplesRootDir/SimpleViewer/Wasm/tsconfig-simple-viewer.json
cp $currentDir/build-wasm/OrthancStoneSimpleViewer.js  $outputDir/simple-viewer/
cp $currentDir/build-wasm/OrthancStoneSimpleViewer.wasm  $outputDir/simple-viewer/

cd $currentDir

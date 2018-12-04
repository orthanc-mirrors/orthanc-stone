#!/bin/bash

set -e

target=${1:-all}
# this script currently assumes that the wasm code has been built on its side and is availabie in build-wasm/

currentDir=$(pwd)
samplesRootDir=$(pwd)

outputDir=$samplesRootDir/build-web/
mkdir -p $outputDir

# files used by all single files samples
cp $samplesRootDir/Web/index.html $outputDir
cp $samplesRootDir/Web/samples-styles.css $outputDir

# build simple-viewer-single-file (obsolete project)
if [[ $target == "all" || $target == "OrthancStoneSimpleViewerSingleFile" ]]; then
  cp $samplesRootDir/Web/simple-viewer-single-file.html $outputDir
  tsc --allowJs --project $samplesRootDir/Web/simple-viewer-single-file.tsconfig.json
  cp $currentDir/build-wasm/OrthancStoneSimpleViewerSingleFile.js  $outputDir
  cp $currentDir/build-wasm/OrthancStoneSimpleViewerSingleFile.wasm  $outputDir
fi

# build single-frame
if [[ $target == "all" || $target == "OrthancStoneSingleFrame" ]]; then
  cp $samplesRootDir/Web/single-frame.html $outputDir
  tsc --allowJs --project $samplesRootDir/Web/single-frame.tsconfig.json
  cp $currentDir/build-wasm/OrthancStoneSingleFrame.js  $outputDir
  cp $currentDir/build-wasm/OrthancStoneSingleFrame.wasm  $outputDir
fi

# build single-frame-editor
if [[ $target == "all" || $target == "OrthancStoneSingleFrameEditor" ]]; then
  cp $samplesRootDir/Web/single-frame-editor.html $outputDir
  tsc --allowJs --project $samplesRootDir/Web/single-frame-editor.tsconfig.json
  cp $currentDir/build-wasm/OrthancStoneSingleFrameEditor.js  $outputDir
  cp $currentDir/build-wasm/OrthancStoneSingleFrameEditor.wasm  $outputDir
fi

# build simple-viewer project
if [[ $target == "all" || $target == "OrthancStoneSimpleViewer" ]]; then
  mkdir -p $outputDir/simple-viewer/
  cp $samplesRootDir/SimpleViewer/Wasm/simple-viewer.html $outputDir/simple-viewer/
  cp $samplesRootDir/SimpleViewer/Wasm/styles.css $outputDir/simple-viewer/
  tsc --allowJs --project $samplesRootDir/SimpleViewer/Wasm/tsconfig-simple-viewer.json
  cp $currentDir/build-wasm/OrthancStoneSimpleViewer.js  $outputDir/simple-viewer/
  cp $currentDir/build-wasm/OrthancStoneSimpleViewer.wasm  $outputDir/simple-viewer/
fi

cd $currentDir

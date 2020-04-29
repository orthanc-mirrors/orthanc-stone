#!/bin/bash

set -e

target=${1:-all}
# this script currently assumes that the wasm code has been built on its side and is availabie in build-wasm/

currentDir=$(pwd)
samplesRootDir=$(pwd)

echo "*************************************************************************"
echo "samplesRootDir = $samplesRootDir"
echo "*************************************************************************"

outputDir=$samplesRootDir/build-web/
mkdir -p "$outputDir"

# files used by all single files samples
cp "$samplesRootDir/Web/index.html" "$outputDir"
cp "$samplesRootDir/Web/samples-styles.css" "$outputDir"

# # build simple-viewer-single-file (obsolete project)
# if [[ $target == "all" || $target == "OrthancStoneSimpleViewerSingleFile" ]]; then
#   cp $samplesRootDir/Web/simple-viewer-single-file.html $outputDir
#   tsc --project $samplesRootDir/Web/simple-viewer-single-file.tsconfig.json --outDir "$outputDir"
#   browserify \
#       "$outputDir/Platforms/Wasm/wasm-application-runner.js" \
#       "$outputDir/Applications/Samples/Web/simple-viewer-single-file.js" \
#       -o "$outputDir/app-simple-viewer-single-file.js"
#   cp "$currentDir/build-wasm/OrthancStoneSimpleViewerSingleFile.js"  $outputDir
#   cp "$currentDir/build-wasm/OrthancStoneSimpleViewerSingleFile.wasm"  $outputDir
# fi

# # build single-frame
# if [[ $target == "all" || $target == "OrthancStoneSingleFrame" ]]; then
#   cp $samplesRootDir/Web/single-frame.html $outputDir
#   tsc --project $samplesRootDir/Web/single-frame.tsconfig.json --outDir "$outputDir"
#   browserify \
#       "$outputDir/Platforms/Wasm/wasm-application-runner.js" \
#       "$outputDir/Applications/Samples/Web/single-frame.js" \
#       -o "$outputDir/app-single-frame.js"
#   cp "$currentDir/build-wasm/OrthancStoneSingleFrame.js"  $outputDir
#   cp "$currentDir/build-wasm/OrthancStoneSingleFrame.wasm"  $outputDir
# fi

# build single-frame-editor
if [[ $target == "all" || $target == "OrthancStoneSingleFrameEditor" ]]; then
  cp $samplesRootDir/Web/single-frame-editor.html $outputDir
  tsc --project $samplesRootDir/Web/single-frame-editor.tsconfig.json --outDir "$outputDir"
  browserify \
      "$outputDir/Platforms/Wasm/wasm-application-runner.js" \
      "$outputDir/Applications/Samples/Web/single-frame-editor.js" \
      -o "$outputDir/app-single-frame-editor.js"
  cp "$currentDir/build-wasm/OrthancStoneSingleFrameEditor.js"  $outputDir
  cp "$currentDir/build-wasm/OrthancStoneSingleFrameEditor.wasm"  $outputDir
fi

# build simple-viewer project
if [[ $target == "all" || $target == "OrthancStoneSimpleViewer" ]]; then
  mkdir -p $outputDir/simple-viewer/
  cp $samplesRootDir/SimpleViewer/Wasm/simple-viewer.html $outputDir/simple-viewer/
  cp $samplesRootDir/SimpleViewer/Wasm/styles.css $outputDir/simple-viewer/
  
  # the root dir must contain all the source files for the whole project
  tsc --module commonjs --allowJs --project "$samplesRootDir/SimpleViewer/Wasm/tsconfig-simple-viewer.json" --rootDir "$samplesRootDir/../.." --outDir "$outputDir/simple-viewer/"
  browserify \
    "$outputDir/simple-viewer/Platforms/Wasm/wasm-application-runner.js" \
    "$outputDir/simple-viewer/Applications/Samples/SimpleViewer/Wasm/simple-viewer.js" \
    -o "$outputDir/simple-viewer/app-simple-viewer.js"
  cp "$currentDir/build-wasm/OrthancStoneSimpleViewer.js"  "$outputDir/simple-viewer/"
  cp "$currentDir/build-wasm/OrthancStoneSimpleViewer.wasm"  "$outputDir/simple-viewer/"
fi

cd $currentDir

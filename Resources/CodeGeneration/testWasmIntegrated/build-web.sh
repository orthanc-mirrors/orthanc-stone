#!/bin/bash
set -e

mkdir -p build-final

# compile TS to JS
tsc --module commonjs --sourceMap -t ES2015 --outDir "build-tsc/" build-wasm/testWasmIntegratedCpp_generated.ts testWasmIntegrated.ts 

# bundle JS files to final build dir 
browserify "build-tsc/build-wasm/testWasmIntegratedCpp_generated.js" "build-tsc/testWasmIntegrated.js" -o "build-final/testWasmIntegratedApp.js"

# copy WASM loader JS file to final build dir 
cp build-wasm/testWasmIntegratedCpp.js build-final/

# copy HTML start page to output dir
cp testWasmIntegrated.html build-final/


# copy styles to output dir
cp styles.css build-final/

# copy WASM binary to output dir
cp build-wasm/testWasmIntegratedCpp.wasm  build-final/

echo "...Serving files at http://127.0.0.1:8080/"
echo "Please open build_final/testWasmIntegrated.html"

sudo python3 serve.py


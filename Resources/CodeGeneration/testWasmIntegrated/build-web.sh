#!/bin/bash
set -e

mkdir -p build-final

# compile TS to JS
tsc --module commonjs --sourceMap -t ES2015 --outDir "build-tsc/" build-wasm/testWasmIntegratedCpp_generated.ts testWasmIntegrated.ts 

# bundle all JS files to final build dir 
browserify "build-wasm/testWasmIntegratedCpp.js" "build-tsc/build-wasm/testWasmIntegratedCpp_generated.js" "build-tsc/testWasmIntegrated.js" -o "build-final/testWasmIntegratedApp.js"

# copy HTML start page to output dir
cp testWasmIntegrated.html build-final/

# copy styles to output dir
cp styles.css build-final/

# copy WASM binary to output dir
cp build-wasm/testWasmIntegratedCpp.wasm  build-final/

sudo python3 serve.py


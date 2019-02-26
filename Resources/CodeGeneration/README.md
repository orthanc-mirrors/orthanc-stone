Requirements
----------------

Install Node and npm.

Then:
- `npm install browserify`
- `npm install typescript`
- `npm install tsify`

`testCppHandler` contains a C++ project that produces an executable 
slurping a set of text files representing messages defined against 
the `test_data/test1.yaml' schema and dumping them to `cout`.

'testWasmIntegrated` contains a small Web app demonstrating the 
interaction between TypeScript and C++ in WASM.
source ~/apps/emsdk/emsdk_env.sh
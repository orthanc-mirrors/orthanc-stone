# Path to this plugin 

# Under Linux
# $PROTOC_GEN_TS_PATH="$PSScriptRoot/node_modules/.bin/protoc-gen-ts"

# Under Windows
$PROTOC_GEN_TS_PATH="$PSScriptRoot/node_modules/.bin/protoc-gen-ts.cmd"

# Directory to write generated code to (.js and .d.ts files) 
$OUT_DIR="./generated_ts"
 
protoc `
    --plugin="protoc-gen-ts=$PROTOC_GEN_TS_PATH" `
    --js_out="import_style=commonjs,binary:$OUT_DIR" `
    --ts_out="$OUT_DIR" `
    basic.proto



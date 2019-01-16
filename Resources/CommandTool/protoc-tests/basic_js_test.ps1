# Path to this plugin 

# Under Linux
# $PROTOC_GEN_TS_PATH="$PSScriptRoot/node_modules/.bin/protoc-gen-ts"

# Under Windows

# Directory to write generated code to (.js and .d.ts files) 
$OUT_DIR="./generated_js"
 
protoc `
    --js_out="$OUT_DIR" `
    basic.proto



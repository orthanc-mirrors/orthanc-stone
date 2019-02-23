# echo "+----------------------+"
# echo "|    template.in.ts    |"
# echo "+----------------------+"

# tsc -t ES2015 .\template.in.ts; node .\template.in.js

# echo "+----------------------+"
# echo "|    playground.ts     |"
# echo "+----------------------+"

# tsc -t ES2015 .\playground.ts; node .\playground.js

# echo "+----------------------+"
# echo "|    playground3.ts     |"
# echo "+----------------------+"

# tsc -t ES2015 .\playground3.ts; node .\playground3.js

echo "+----------------------+"
echo "|      stonegen        |"
echo "+----------------------+"

if(-not (test-Path "build")) {
  mkdir "build"
}

echo "Generate the TS and CPP wrapper... (to build/)"
python stonegentool.py -o "." test_data/test1.yaml
if($LASTEXITCODE -ne 0) {
  Write-Error ("Code generation failed!")
  exit $LASTEXITCODE
}

echo "Compile the TS wrapper to JS... (in build/)"
tsc --module commonjs --sourceMap -t ES2015 --outDir "build/" VsolMessages_generated.ts
if($LASTEXITCODE -ne 0) {
  Write-Error ("Code compilation failed!")
  exit $LASTEXITCODE
}

echo "Compile the test app..."
tsc --module commonjs --sourceMap -t ES2015 --outDir "build/" test_stonegen.ts
if($LASTEXITCODE -ne 0) {
  Write-Error ("Code compilation failed!")
  exit $LASTEXITCODE
}

browserify "build/test_stonegen.js" "build/VsolMessages_generated.js" -o "build_browser/test_stonegen_fused.js"

cp .\test_stonegen.html .\build_browser\

echo "Run the test app..."
Push-Location
cd build_browser
node .\test_stonegen_fused.js
Pop-Location
if($LASTEXITCODE -ne 0) {
  Write-Error ("Code execution failed!")
  exit $LASTEXITCODE
}




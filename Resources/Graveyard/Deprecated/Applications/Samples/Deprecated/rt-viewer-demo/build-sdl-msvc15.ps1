if (-not (Test-Path "build-sdl-msvc15")) {
  mkdir -p "build-sdl-msvc15"
}

cd build-sdl-msvc15

cmake -G "Visual Studio 15 2017 Win64" -DSTATIC_BUILD=ON -DOPENSSL_NO_CAPIENG=ON -DORTHANC_FRAMEWORK_SOURCE=path -DSTONE_SOURCES_DIR="$($pwd)\..\..\..\.." -DORTHANC_FRAMEWORK_ROOT="$($pwd)\..\..\..\..\..\orthanc" -DALLOW_DOWNLOADS=ON -DENABLE_SDL=ON .. 

if (!$?) {
	Write-Error 'cmake configuration failed' -ErrorAction Stop
}

cmake --build . --target RtViewerDemo --config Debug

if (!$?) {
	Write-Error 'cmake build failed' -ErrorAction Stop
}

cd Debug

.\RtViewerDemo.exe --ct-series=a04ecf01-79b2fc33-58239f7e-ad9db983-28e81afa --dose-instance=830a69ff-8e4b5ee3-b7f966c8-bccc20fb-d322dceb --struct-instance=54460695-ba3885ee-ddf61ac0-f028e31d-a6e474d9




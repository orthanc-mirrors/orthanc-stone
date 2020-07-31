
**These samples are deprecated and not guaranteed to work. A migration to a 
new version of the Stone API is underway.**

Please read orthanc-stone/OrthancStone/Samples/README.md first for general requirements.



Deprecated -- to be sorted
===========================

The following assumes that the source code to be downloaded in
`~/orthanc-stone` and Orthanc source code to be checked out in
`~/orthanc`. 

Building the WASM samples
-------------------------------------

```
cd ~/orthanc-stone/Applications/Samples
./build-wasm.sh
```

Serving the WASM samples
------------------------------------
```
# launch an Orthanc listening on 8042 port:
Orthanc

# launch an nginx that will serve the WASM static files and reverse
# proxy
sudo nginx -p $(pwd) -c nginx.local.conf
```

You can now open the samples in http://localhost:9977

Building the SDL native samples (SimpleViewer only)
---------------------------------------------------

The following also assumes that you have checked out the Orthanc
source code in an `orthanc` folder next to the Stone of Orthanc
repository, please enter the following:

**Simple make generator with dynamic build**

```
# Please set $currentDir to the current folder
mkdir -p ~/builds/orthanc-stone-build
cd ~/builds/orthanc-stone-build
cmake -DORTHANC_FRAMEWORK_SOURCE=path \
    -DORTHANC_FRAMEWORK_ROOT=$currentDir/../../../orthanc \
    -DALLOW_DOWNLOADS=ON -DENABLE_SDL=ON \
    ~/orthanc-stone/Applications/Samples/
```

**Ninja generator with static SDL build (pwsh script)**

```
# Please yourself one level above the orthanc-stone and orthanc folders
if( -not (test-path stone_build_sdl)) { mkdir stone_build_sdl }
cd stone_build_sdl
cmake -G Ninja -DSTATIC_BUILD=ON -DOPENSSL_NO_CAPIENG=ON -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT="$($pwd)\..\orthanc" -DALLOW_DOWNLOADS=ON -DENABLE_SDL=ON ../orthanc-stone/Applications/Samples/
```

**Ninja generator with static SDL build (bash/zsh script)**

```
# Please yourself one level above the orthanc-stone and orthanc folders
if( -not (test-path stone_build_sdl)) { mkdir stone_build_sdl }
cd stone_build_sdl
cmake -G Ninja -DSTATIC_BUILD=ON -DOPENSSL_NO_CAPIENG=ON -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT="`pwd`/../orthanc" -DALLOW_DOWNLOADS=ON -DENABLE_SDL=ON ../orthanc-stone/Applications/Samples/
```

**Visual Studio 2017 generator with static SDL build  (pwsh script)**

```
# The following will use Visual Studio 2017 to build the SDL samples
# in debug mode (with multiple compilers in parallel). NOTE: place 
# yourself one level above the `orthanc-stone` and `orthanc` folders

if( -not (test-path stone_build_sdl)) { mkdir stone_build_sdl }
cd stone_build_sdl
cmake -G "Visual Studio 15 2017 Win64" -DMSVC_MULTIPLE_PROCESSES=ON -DSTATIC_BUILD=ON -DOPENSSL_NO_CAPIENG=ON -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT="$($pwd)\..\orthanc" -DALLOW_DOWNLOADS=ON -DENABLE_SDL=ON ../orthanc-stone/Applications/Samples/
cmake --build . --config Debug
```

If you are working on Windows, add the correct generator option to
cmake to, for instance, generate msbuild files for Visual Studio.

Then, under Linux:
```
cmake --build . --target OrthancStoneSimpleViewer -- -j 5
```

Note: replace `$($pwd)` with the current directory when not using Powershell

Building the Qt native samples (SimpleViewer only) under Windows:
------------------------------------------------------------------

**Visual Studio 2017 generator with static Qt build  (pwsh script)**

For instance, if Qt is installed in `C:\Qt\5.12.0\msvc2017_64`

```
# The following will use Visual Studio 2017 to build the SDL samples
# in debug mode (with multiple compilers in parallel). NOTE: place 
# yourself one level above the `orthanc-stone` and `orthanc` folders

if( -not (test-path stone_build_qt)) { mkdir stone_build_qt }
cd stone_build_qt
cmake -G "Visual Studio 15 2017 Win64" -DMSVC_MULTIPLE_PROCESSES=ON -DSTATIC_BUILD=ON -DOPENSSL_NO_CAPIENG=ON -DCMAKE_PREFIX_PATH=C:\Qt\5.12.0\msvc2017_64 -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT="$($pwd)\..\orthanc" -DALLOW_DOWNLOADS=ON -DENABLE_QT=ON  ../orthanc-stone/Applications/Samples/
cmake --build . --config Debug
```

Note: replace `$($pwd)` with the current directory when not using Powershell






Building the SDL native samples (SimpleViewer only) under Windows:
------------------------------------------------------------------
`cmake -DSTATIC_BUILD=ON -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT="$($pwd)\..\orthanc" -DALLOW_DOWNLOADS=ON -DENABLE_SDL=ON -G "Visual Studio 15 2017 Win64" ../orthanc-stone/Applications/Samples/`

Note: replace `$($pwd)` with the current directory when not using Powershell

Executing the native samples:
--------------------------------
```
# launch an Orthanc listening on 8042 port:
Orthanc

# launch the sample
./OrthancStoneSimpleViewer --studyId=XX
``` 

Build the Application Samples
-----------------------------

**Visual Studio 2008 (v90) **

```
cmake -G "Visual Studio 9 2008" -DUSE_LEGACY_JSONCPP=ON -DENABLE_OPENGL=ON -DSTATIC_BUILD=ON -DOPENSSL_NO_CAPIENG=ON -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT="$($pwd)\..\orthanc" -DALLOW_DOWNLOADS=ON -DENABLE_SDL=ON ../orthanc-stone/Applications/Samples
```

**Visual Studio 2019 (v142) **

```
cmake -G "Visual Studio 16 2019" -A x64 -DMSVC_MULTIPLE_PROCESSES=ON -DENABLE_OPENGL=ON -DSTATIC_BUILD=ON -DOPENSSL_NO_CAPIENG=ON -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT="$($pwd)\..\orthanc" -DALLOW_DOWNLOADS=ON -DENABLE_SDL=ON ../orthanc-stone/Applications/Samples
```

**Visual Studio 2017 (v140) **

```
cmake -G "Visual Studio 15 2017 Win64" -DMSVC_MULTIPLE_PROCESSES=ON -DENABLE_OPENGL=ON -DSTATIC_BUILD=ON -DOPENSSL_NO_CAPIENG=ON -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT="$($pwd)\..\orthanc" -DALLOW_DOWNLOADS=ON -DENABLE_SDL=ON ../orthanc-stone/Applications/Samples
```


Build the core Samples
---------------------------
How to build the newest (2019-04-29) SDL samples under Windows, *inside* a
folder that is sibling to the orthanc-stone folder: 

**Visual Studio 2019 (v142) **

```
cmake -G "Visual Studio 16 2019" -A x64 -DMSVC_MULTIPLE_PROCESSES=ON -DENABLE_OPENGL=ON -DSTATIC_BUILD=ON -DOPENSSL_NO_CAPIENG=ON -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT="$($pwd)\..\orthanc" -DALLOW_DOWNLOADS=ON -DENABLE_SDL=ON ../orthanc-stone/OrthancStone/Samples/Sdl
```

**Visual Studio 2017 (v140) **

```
cmake -G "Visual Studio 15 2017 Win64" -DMSVC_MULTIPLE_PROCESSES=ON -DENABLE_OPENGL=ON -DSTATIC_BUILD=ON -DOPENSSL_NO_CAPIENG=ON -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT="$($pwd)\..\orthanc" -DALLOW_DOWNLOADS=ON -DENABLE_SDL=ON ../orthanc-stone/OrthancStone/Samples/Sdl
```

**Visual Studio 2008 (v90) **

```
cmake -G "Visual Studio 9 2008" -DUSE_LEGACY_JSONCPP=ON -DENABLE_OPENGL=ON -DSTATIC_BUILD=ON -DOPENSSL_NO_CAPIENG=ON -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT="$($pwd)\..\orthanc" -DALLOW_DOWNLOADS=ON -DENABLE_SDL=ON ../orthanc-stone/OrthancStone/Samples/Sdl
```

And under Ubuntu (note the /mnt/c/osi/dev/orthanc folder):
```
cmake -G "Ninja" -DENABLE_OPENGL=ON -DSTATIC_BUILD=OFF -DOPENSSL_NO_CAPIENG=ON -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT="/mnt/c/osi/dev/orthanc" -DALLOW_DOWNLOADS=ON -DENABLE_SDL=ON ../orthanc-stone/OrthancStone/Samples/Sdl
```

TODO trackers:
- CANCELLED (using outlined text now) text overlay 50% --> ColorTextureLayer 50%
- DONE angle tracker: draw arcs
- Handles on arc
- Select measure tool with hit test --> Delete command




General
=======
These samples assume that a recent version of Orthanc is checked out in an
`orthanc` folder next to the `orthanc-stone` folder. Let's call the top folder
the `devroot` folder. This name does not matter and is not used anywhere.

Here's the directory layout that we suggest:

```
devroot/
 |
 +- orthanc/
 |
 +- orthanc-stone/
 |
 ...
```

 Orthanc can be retrieved with:
 ```
 hg clone https://hg.orthanc-server.com/orthanc
 ```

Furthermore, the samples usually assume that an Orthanc is running locally,
without authentication, on port 8042. The samples can easily be tweaked if 
your setup is different.

When Dicom resources are to be displayed, their IDs can be supplied in the 
various ways suitable for the platform (command-line arguments, URL parameters
or through the GUI)

 
WebAssembly samples
===================

Building the WebAssembly samples require the Emscripten SDK 
(https://emscripten.org/). This SDK goes far beyond the simple compilation to
the wasm (Web Assembly) bytecode and provides a comprehensive library that 
eases porting native C and C++ programs and libraries. The Emscripten SDK also
makes it easy to generate the companion Javascript files requires to use a 
wasm module in a web application.

Although Emscripten runs on all major platforms, Stone of Orthanc is developed
and tested with the Linux version of Emscripten.

Emscripten runs perfectly fine under the Windows Subsystem for Linux (that is
the environment used quite often by the Stone of Orthanc team)

**Important note:** The following examples **and the build scripts** will 
assume that you have installed the Emscripten SDK in `~/apps/emsdk`.

The following packages should get you going (a Debian-like distribution such 
as Debian or Ubuntu is assumed)

```
sudo apt-get update 
sudo apt-get install -y build-essential curl wget git python cmake pkg-config
sudo apt-get install -y mercurial unzip npm ninja-build p7zip-full gettext-base 
```

SingleFrameViewer
-----------------

This sample application displays a single frame of a Dicom instance that can
be loaded from Orthanc, either by using the Orthanc REST API or through the 
Dicomweb server functionality of Orthanc.

This barebones sample uses plain Javascript and requires the 
Emscripten toolchain and cmake, in addition to a few standard packages.

Here's how you can build it: create the following script (for instance, 
`build-wasm-SingleFrameViewer.sh`) one level above the orthanc-stone repository,
thus, in the folder we have called `devroot`.

If you feel confident, you can also simply read the following script and 
enter the commands interactively in the terminal.

```
#!/bin/bash

if [ ! -d "orthanc-stone" ]; then
  echo "This script must be run from the folder one level above orthanc-stone"
  exit 1
fi

if [[ ! $# -eq 1 ]]; then
  echo "Usage:"
  echo "  $0 [BUILD_TYPE]"
  echo ""
  echo "  with:"
  echo "    BUILD_TYPE = Debug, RelWithDebInfo or Release"
  echo ""
  exit 1
fi

# define the variables that we'll use
buildType=$1
buildFolderName="`pwd`/out/build-stone-wasm-SingleFrameViewer-$buildType"
installFolderName="`pwd`/out/install-stone-wasm-SingleFrameViewer-$buildType"

# configure the environment to use Emscripten
. ~/apps/emsdk/emsdk_env.sh


mkdir -p $buildFolderName

# change current folder to the build folder
pushd $buildFolderName

emcmake cmake -G "Ninja" \
  -DCMAKE_BUILD_TYPE=$buildType \
  -DCMAKE_INSTALL_PREFIX=$installFolderName \
  -DSTATIC_BUILD=ON -DOPENSSL_NO_CAPIENG=ON -DALLOW_DOWNLOADS=ON \
  ../orthanc-stone/Samples/WebAssembly/SingleFrameViewer

# perform build + installation
ninja install

# restore the original working folder
popd

echo "If all went well, the output files can be found in $installFolderName:"

ls $installFolderName```
```

Simply navigate to the dev root, and execute the script:

```
./build-wasm-SingleFrameViewer.sh RelWithDebInfo
```

I suggest that you do *not* use the `Debug` confirmation unless you really 
need it, for the additional checks that are made will lead to a very long 
build time and much slower execution (more severe than with a native non-wasm
target)

Native samples
=================

SdlSimpleViewer
---------------

### Windows build 

Here's how to build the SdlSimpleViewer example using Visual Studio 2019
(the shell is Powershell, but the legacy shell can also be used with some 
tweaks). This example is meant to be launched from the folder above 
orthanc-stone.

```
  # create the build folder and navigate to it
  $buildDir = "build-stone-sdlviewer-msvc16-x64"

  if (-not (Test-Path $buildDir)) {
    mkdir -p $buildDir | Out-Null
  }
  
  cd $buildDir
  
  # perform the configuration
  cmake -G "Visual Studio 16 2019" -A x64 `
    -DMSVC_MULTIPLE_PROCESSES=ON `
    -DALLOW_DOWNLOADS=ON `
    -DSTATIC_BUILD=ON `
    -DOPENSSL_NO_CAPIENG=ON `
    ../orthanc-stone/Samples/Sdl/SimpleViewer
  
  $solutionPath = ls -filter *.sln
  Write-Host "Solution file(s) available at: $solutionPath"
```

The initial configuration step will be quite lengthy, for CMake needs to 
setup its internal cache based on your environment and build tools.

Subsequent runs will be several orders of magnitude faster!

One the solution (.sln) file is ready, you can open it using the Visual Studio
IDE and choose Build --> Build solution.

An alternative is to execute `cmake --build .` in the build folder created by
the script.


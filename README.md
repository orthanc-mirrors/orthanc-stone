Stone of Orthanc
================

General Information
-------------------

This repository contains the source code of the Stone of Orthanc.

Stone of Orthanc is a lightweight, cross-platform C++ framework for
the CPU-based rendering of medical images. It notably features support
for MPR (multiplanar reconstruction of volume images), PET-CT fusion,
and accurate physical world coordinates.

Stone of Orthanc comes bundled with its own software-based rendering
engine (based upon pixman). This engine will use CPU hardware
acceleration if available (notably SSE2, SSSE3, and NEON instruction
sets), but not the GPU. This makes Stone a highly versatile framework
that can run even on low-performance platforms. Note that Stone is
able to display DICOM series without having to entirely store them in
the RAM (i.e. frame by frame).

Thanks to its standalone rendering engine, Stone of Orthanc is also
compatible with any GUI framework (such as Qt, wxWidgets, MFC...). The
provided sample applications use the SDL framework.

Stone is conceived as a companion toolbox to the Orthanc VNA (vendor
neutral archive, i.e. DICOM server). As a consequence, Stone will
smoothly interface with Orthanc out of the box. Interestingly, Stone
does not contain any DICOM toolkit: It entirely relies on the REST API
of Orthanc to parse/decode DICOM images. However, thanks to the
object-oriented architecture of Stone, it is possible to avoid this
dependency upon Orthanc, e.g. to download DICOM datasets using
DICOMweb.


Comparison
----------

Pay attention to the fact that Stone of Orthanc is a toolkit, and not
a fully-featured application for the visualization of medical images
(such as Horos/OsiriX or Ginkgo CADx). However, such applications
can be built on the top of Stone of Orthanc.

Stone of Orthanc is quite similar to two other well-known toolkits:

* Cornerstone, a client-side JavaScript toolkit to display medical
  images in Web browsers, by Chris Hafey <chafey@gmail.com>:
  https://github.com/chafey/cornerstone

  Contrarily to Cornerstone, Stone of Orthanc can be embedded into
  native, heavyweight applications.

* VTK, a C++ toolkit for scientific visualization, by Kitware:
  http://www.vtk.org/

  Contrarily to VTK, Stone of Orthanc is focused on CPU-based, 2D
  rendering: The GPU will not be used.


Dependencies
------------

Stone of Orthanc is based upon the following projects:

* Orthanc, a lightweight Vendor Neutral Archive (DICOM server):
  http://www.orthanc-server.com/

* Cairo and pixman, a cross-platform 2D graphics library:
  https://www.cairographics.org/

* Optionally, SDL, a cross-platform multimedia library:
  https://www.libsdl.org/

Prerequisites to compile natively on Ubuntu: 
```
sudo apt-get install -y libcairo-dev libpixman-1-dev libsdl2-dev
```

The emscripten SDK is required for the WASM build. Please install it
in `~/apps/emsdk`. If you wish to use it in another way, please edit
the `build-wasm.sh` file.

ninja (`sudo apt-get install -y ninja-build`) is used instead of make, for performance reasons.

Installation and usage ----------------------

Build instructions are similar to that of Orthanc:
http://book.orthanc-server.com/faq/compiling.html

Usage details are available as part of the Orthanc Book:
http://book.orthanc-server.com/developers/stone.html

Stone of Orthanc comes with several sample applications in the
`Samples` folder. These samples can be compiled into Web Assembly or
into native SDL applications.

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

Licensing
---------

Stone of Orthanc is licensed under the AGPL license.

We also kindly ask scientific works and clinical studies that make
use of Orthanc to cite Orthanc in their associated publications.
Similarly, we ask open-source and closed-source products that make
use of Orthanc to warn us about this use. You can cite our work
using the following BibTeX entry:

@Article{Jodogne2018,
  author="Jodogne, S{\'e}bastien",
  title="The {O}rthanc Ecosystem for Medical Imaging",
  journal="Journal of Digital Imaging",
  year="2018",
  month="Jun",
  day="01",
  volume="31",
  number="3",
  pages="341--352",
  issn="1618-727X",
  doi="10.1007/s10278-018-0082-y",
  url="https://doi.org/10.1007/s10278-018-0082-y"
}

Build the Application Samples
-----------------------------

**Visual Studio 2008 (v90) **

```
cmake -G "Visual Studio 9 2008" -DUSE_LEGACY_JSONCPP=ON -DENABLE_OPENGL=ON -DSTATIC_BUILD=ON -DOPENSSL_NO_CAPIENG=ON -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT="$($pwd)\..\orthanc" -DALLOW_DOWNLOADS=ON -DENABLE_SDL=ON ../orthanc-stone/Applications/Samples
```

Various notes to be sorted
---------------------------
How to build the newest (2019-04-29) SDL samples under Windows, *inside* a
folder that is sibling to the orthanc-stone folder: 

**Visual Studio 2017 (v140) **

```
cmake -G "Visual Studio 15 2017 Win64" -DMSVC_MULTIPLE_PROCESSES=ON -DENABLE_OPENGL=ON -DSTATIC_BUILD=ON -DOPENSSL_NO_CAPIENG=ON -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT="$($pwd)\..\orthanc" -DALLOW_DOWNLOADS=ON -DENABLE_SDL=ON ../orthanc-stone/Samples/Sdl
```

**Visual Studio 2008 (v90) **

```
cmake -G "Visual Studio 9 2008" -DUSE_LEGACY_JSONCPP=ON -DENABLE_OPENGL=ON -DSTATIC_BUILD=ON -DOPENSSL_NO_CAPIENG=ON -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT="$($pwd)\..\orthanc" -DALLOW_DOWNLOADS=ON -DENABLE_SDL=ON ../orthanc-stone/Samples/Sdl
```

And under Ubuntu (note the /mnt/c/osi/dev/orthanc folder):
```
cmake -G "Ninja" -DENABLE_OPENGL=ON -DSTATIC_BUILD=OFF -DOPENSSL_NO_CAPIENG=ON -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT="/mnt/c/osi/dev/orthanc" -DALLOW_DOWNLOADS=ON -DENABLE_SDL=ON ../orthanc-stone/Samples/Sdl
```

TODO trackers:
- CANCELLED (using outlined text now) text overlay 50% --> ColorTextureLayer 50%
- DONE angle tracker: draw arcs
- Handles on arc
- Select measure tool with hit test --> Delete command


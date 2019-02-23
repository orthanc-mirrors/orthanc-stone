Requirements
==============
- Install Python 3.x (tested with 3.7)
- Install conan with `pip install conan` (tested with 1.12.2)
- Install CMake (tested with 3.12)
- Under Windows: Visual Studio 2017
- Under *nix*: Ninja

How to build under *nix*
===============================
- Navigate to `testCppHandler` folder
- `conan install . -g cmake`
- `mkdir build`
- `cd build`
- `cmake -G "Ninja" ..`
- `cmake --build . --config Debug` or - `cmake --build . --config Release`

How to build under Windows with Visual Studio
==============================================
- Navigate to repo root
- `mkdir build`
- `cd build`
- `conan install .. -g cmake_multi -s build_type=Release`
- `conan install .. -g cmake_multi -s build_type=Debug`
- `cmake -G "Visual Studio 15 2017 Win64" ..`  (modify for your current Visual Studio version)
- `cmake --build . --config Debug` or - `cmake --build . --config Release`







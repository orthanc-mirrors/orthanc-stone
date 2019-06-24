cmake_minimum_required(VERSION 2.8.3)


#####################################################################
## Configuration of the Emscripten compiler for WebAssembly target
#####################################################################

set(WASM_FLAGS "-s WASM=1 -s FETCH=1")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WASM_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WASM_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXTRA_EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\"]'")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ERROR_ON_UNDEFINED_SYMBOLS=1")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ALLOW_MEMORY_GROWTH=1")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ASSERTIONS=1 -s DISABLE_EXCEPTION_CATCHING=0")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXIT_RUNTIME=1")


#####################################################################
## Configuration of the Orthanc framework
#####################################################################

# This CMake file defines the "ORTHANC_STONE_VERSION" macro, so it
# must be the first inclusion
include(${CMAKE_SOURCE_DIR}/../../Resources/CMake/Version.cmake)

if (ORTHANC_STONE_VERSION STREQUAL "mainline")
  set(ORTHANC_FRAMEWORK_VERSION "mainline")
  set(ORTHANC_FRAMEWORK_DEFAULT_SOURCE "hg")
else()
  set(ORTHANC_FRAMEWORK_VERSION "1.5.7")
  set(ORTHANC_FRAMEWORK_DEFAULT_SOURCE "web")
endif()

set(ORTHANC_FRAMEWORK_SOURCE "${ORTHANC_FRAMEWORK_DEFAULT_SOURCE}" CACHE STRING "Source of the Orthanc source code (can be \"hg\", \"archive\", \"web\" or \"path\")")
set(ORTHANC_FRAMEWORK_ARCHIVE "" CACHE STRING "Path to the Orthanc archive, if ORTHANC_FRAMEWORK_SOURCE is \"archive\"")
set(ORTHANC_FRAMEWORK_ROOT "" CACHE STRING "Path to the Orthanc source directory, if ORTHANC_FRAMEWORK_SOURCE is \"path\"")


#####################################################################
## Configuration of the Stone framework
#####################################################################

include(${CMAKE_SOURCE_DIR}/../../Resources/CMake/OrthancStoneParameters.cmake)
include(${ORTHANC_ROOT}/Resources/CMake/DownloadPackage.cmake)

DownloadPackage(
  "a24b8136b8f3bb93f166baf97d9328de"
  "http://orthanc.osimis.io/ThirdPartyDownloads/ubuntu-font-family-0.83.zip"
  "${CMAKE_BINARY_DIR}/ubuntu-font-family-0.83")

set(ORTHANC_STONE_APPLICATION_RESOURCES
  UBUNTU_FONT  ${CMAKE_BINARY_DIR}/ubuntu-font-family-0.83/Ubuntu-R.ttf
  )

SET(ENABLE_GOOGLE_TEST OFF)
SET(ENABLE_LOCALE ON)
SET(ORTHANC_SANDBOXED ON)
SET(ENABLE_WASM ON)

include(${CMAKE_SOURCE_DIR}/../../Resources/CMake/OrthancStoneConfiguration.cmake)

add_definitions(
  -DORTHANC_ENABLE_LOGGING_PLUGIN=0
  )


#####################################################################
## Build the samples
#####################################################################

add_library(OrthancStone STATIC
  ${ORTHANC_STONE_SOURCES}
  )


if (ON)
  add_executable(BasicScene
    BasicScene.cpp
    )

  target_link_libraries(BasicScene OrthancStone)

  install(
    TARGETS BasicScene
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
    )
endif()


if (ON)
  add_executable(BasicMPR
    BasicMPR.cpp
    )

  target_link_libraries(BasicMPR OrthancStone)

  install(
    TARGETS BasicMPR
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
    )
endif()
  

install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/BasicMPR.wasm
  ${CMAKE_CURRENT_BINARY_DIR}/BasicScene.wasm
  ${CMAKE_SOURCE_DIR}/BasicMPR.html
  ${CMAKE_SOURCE_DIR}/BasicScene.html
  ${CMAKE_SOURCE_DIR}/Configuration.json
  ${CMAKE_SOURCE_DIR}/app.js
  ${CMAKE_SOURCE_DIR}/index.html
  DESTINATION ${CMAKE_INSTALL_PREFIX}
  )

cmake_minimum_required(VERSION 2.8.3)
project(CtPetDoseStructFusion)

include(../../../Resources/CMake/OrthancStoneParameters.cmake)

if (OPENSSL_NO_CAPIENG)
add_definitions(-DOPENSSL_NO_CAPIENG=1)
endif()

set(ENABLE_SDL OFF CACHE BOOL "Target SDL Native application")
set(ENABLE_QT OFF CACHE BOOL "Target Qt Native application")
set(ENABLE_WASM OFF CACHE BOOL "Target WASM application")

if (ENABLE_WASM)
  #####################################################################
  ## Configuration of the Emscripten compiler for WebAssembly target
  #####################################################################

  set(WASM_FLAGS "-s WASM=1")
  set(WASM_FLAGS "${WASM_FLAGS} -s STRICT=1") # drops support for all deprecated build options
  set(WASM_FLAGS "${WASM_FLAGS} -s FILESYSTEM=1") # if we don't include it, gen_uuid.c fails to build because srand, getpid(), ... are not defined
  set(WASM_FLAGS "${WASM_FLAGS} -s DISABLE_EXCEPTION_CATCHING=0") # actually enable exception catching 
  set(WASM_FLAGS "${WASM_FLAGS} -s ERROR_ON_MISSING_LIBRARIES=1")

  if (CMAKE_BUILD_TYPE MATCHES DEBUG)
    set(WASM_FLAGS "${WASM_FLAGS} -g4") # generate debug information
    set(WASM_FLAGS "${WASM_FLAGS} -s ASSERTIONS=2") # more runtime checks
  else()
    set(WASM_FLAGS "${WASM_FLAGS} -Os") # optimize for web (speed and size)
  endif()

  set(WASM_MODULE_NAME "StoneFrameworkModule" CACHE STRING "Name of the WebAssembly module")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WASM_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WASM_FLAGS}")

  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${WASM_FLAGS}")  # not always clear which flags are for the compiler and which one are for the linker -> pass them all to the linker too

  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --js-library ${STONE_SOURCES_DIR}/Platforms/Wasm/WasmWebService.js")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --js-library ${STONE_SOURCES_DIR}/Platforms/Wasm/WasmDelayedCallExecutor.js")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --js-library ${STONE_SOURCES_DIR}/Platforms/Wasm/default-library.js")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXTRA_EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\"]'")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORT_NAME='\"${WASM_MODULE_NAME}\"'")

# +-------------------------------+
# | Commented for now!            |     
# +-------------------------------+
  # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ALLOW_MEMORY_GROWTH=1")
  # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s TOTAL_MEMORY=536870912")
  # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s TOTAL_STACK=128000000")

  add_definitions(-DORTHANC_ENABLE_WASM=1)
  set(ORTHANC_SANDBOXED ON)

elseif (ENABLE_QT OR ENABLE_SDL)

  set(ENABLE_NATIVE ON)
  set(ORTHANC_SANDBOXED OFF)
  set(ENABLE_CRYPTO_OPTIONS ON)
  set(ENABLE_GOOGLE_TEST ON)
  set(ENABLE_WEB_CLIENT ON)

endif()

#####################################################################
## Configuration for Orthanc
#####################################################################

if (ORTHANC_STONE_VERSION STREQUAL "mainline")
  set(ORTHANC_FRAMEWORK_VERSION "mainline")
  set(ORTHANC_FRAMEWORK_DEFAULT_SOURCE "hg")
else()
  set(ORTHANC_FRAMEWORK_VERSION "1.4.1")
  set(ORTHANC_FRAMEWORK_DEFAULT_SOURCE "web")
endif()

set(ORTHANC_FRAMEWORK_SOURCE "${ORTHANC_FRAMEWORK_DEFAULT_SOURCE}" CACHE STRING "Source of the Orthanc source code (can be \"hg\", \"archive\", \"web\" or \"path\")")
set(ORTHANC_FRAMEWORK_ARCHIVE "" CACHE STRING "Path to the Orthanc archive, if ORTHANC_FRAMEWORK_SOURCE is \"archive\"")
set(ORTHANC_FRAMEWORK_ROOT "" CACHE STRING "Path to the Orthanc source directory, if ORTHANC_FRAMEWORK_SOURCE is \"path\"")

#####################################################################
## Build a static library containing the Orthanc Stone framework
#####################################################################


LIST(APPEND ORTHANC_BOOST_COMPONENTS program_options)

include(../../Resources/CMake/OrthancStoneConfiguration.cmake)

add_library(OrthancStone STATIC
  ${ORTHANC_STONE_SOURCES}
  )

#####################################################################
## Build the CDSF applications
#####################################################################

if (ENABLE_QT OR ENABLE_WASM)
  if (ENABLE_QT)
    list(APPEND CDSF_APPLICATION_SOURCES
      ${ORTHANC_STONE_ROOT}/Applications/Samples/CtPetDoseStructFusion/Qt/CtPetDoseStructFusionMainWindow.cpp
      ${ORTHANC_STONE_ROOT}/Applications/Samples/CtPetDoseStructFusion/Qt/CtPetDoseStructFusionMainWindow.ui
      ${ORTHANC_STONE_ROOT}/Applications/Samples/CtPetDoseStructFusion/Qt/mainQt.cpp
    )

    ORTHANC_QT_WRAP_UI(CDSF_APPLICATION_SOURCES
      ${ORTHANC_STONE_ROOT}/Applications/Samples/CtPetDoseStructFusion/Qt/CtPetDoseStructFusionMainWindow.ui
    )

    ORTHANC_QT_WRAP_CPP(CDSF_APPLICATION_SOURCES
      ${ORTHANC_STONE_ROOT}/Applications/Samples/CtPetDoseStructFusion/Qt/CtPetDoseStructFusionMainWindow.h
    )

  elseif (ENABLE_WASM)
    list(APPEND CDSF_APPLICATION_SOURCES
      ${ORTHANC_STONE_ROOT}/Applications/Samples/CtPetDoseStructFusion/Wasm/mainWasm.cpp
      ${ORTHANC_STONE_ROOT}/Applications/Samples/CtPetDoseStructFusion/Wasm/CtPetDoseStructFusionWasmApplicationAdapter.cpp
      ${ORTHANC_STONE_ROOT}/Applications/Samples/CtPetDoseStructFusion/Wasm/CtPetDoseStructFusionWasmApplicationAdapter.h
      ${STONE_WASM_SOURCES}
    )
  endif()

  add_executable(CtPetDoseStructFusionApplication
    ${ORTHANC_STONE_ROOT}/Applications/Samples/CtPetDoseStructFusion/AppStatus.h
    ${ORTHANC_STONE_ROOT}/Applications/Samples/CtPetDoseStructFusion/MainWidgetInteractor.cpp
    ${ORTHANC_STONE_ROOT}/Applications/Samples/CtPetDoseStructFusion/MainWidgetInteractor.h
    ${ORTHANC_STONE_ROOT}/Applications/Samples/CtPetDoseStructFusion/Messages.h
    ${ORTHANC_STONE_ROOT}/Applications/Samples/CtPetDoseStructFusion/CtPetDoseStructFusionApplication.cpp
    ${ORTHANC_STONE_ROOT}/Applications/Samples/CtPetDoseStructFusion/CtPetDoseStructFusionApplication.h
    ${ORTHANC_STONE_ROOT}/Applications/Samples/CtPetDoseStructFusion/ThumbnailInteractor.cpp
    ${ORTHANC_STONE_ROOT}/Applications/Samples/CtPetDoseStructFusion/ThumbnailInteractor.h
    ${CDSF_APPLICATION_SOURCES}
    )
  target_link_libraries(CtPetDoseStructFusionApplication OrthancStone)

endif()










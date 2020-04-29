cmake_minimum_required(VERSION 2.8.3)
project(RtViewerDemo)

if(MSVC)
  add_definitions(/MP)
  if (CMAKE_BUILD_TYPE MATCHES DEBUG)
    add_definitions(/JMC)
  endif()
endif()

message("-------------------------------------------------------------------------------------------------------------------")
message("ORTHANC_FRAMEWORK_ROOT is set to ${ORTHANC_FRAMEWORK_ROOT}")
message("-------------------------------------------------------------------------------------------------------------------")

if(NOT DEFINED ORTHANC_FRAMEWORK_ROOT)
  message(FATAL_ERROR "The location of the Orthanc source repository must be set in the ORTHANC_FRAMEWORK_ROOT CMake variable")
endif()

message("-------------------------------------------------------------------------------------------------------------------")
message("STONE_SOURCES_DIR is set to ${STONE_SOURCES_DIR}")
message("-------------------------------------------------------------------------------------------------------------------")

if(NOT DEFINED STONE_SOURCES_DIR)
  message(FATAL_ERROR "The location of the Stone of Orthanc source repository must be set in the STONE_SOURCES_DIR CMake variable")
endif()

include(${STONE_SOURCES_DIR}/Resources/CMake/OrthancStoneParameters.cmake)

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
  # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --js-library ${STONE_SOURCES_DIR}/Applications/Samples/samples-library.js")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --js-library ${STONE_SOURCES_DIR}/Platforms/Wasm/WasmWebService.js")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --js-library ${STONE_SOURCES_DIR}/Platforms/Wasm/WasmDelayedCallExecutor.js")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --js-library ${STONE_SOURCES_DIR}/Platforms/Wasm/default-library.js")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXTRA_EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\"]'")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORT_NAME='\"${WASM_MODULE_NAME}\"'")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ALLOW_MEMORY_GROWTH=1")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s TOTAL_MEMORY=536870912")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s TOTAL_STACK=128000000")

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

add_definitions(
  -DORTHANC_ENABLE_LOGGING_PLUGIN=0
  )


#####################################################################
## Build a static library containing the Orthanc Stone framework
#####################################################################

LIST(APPEND ORTHANC_BOOST_COMPONENTS program_options)

include(${STONE_SOURCES_DIR}/Resources/CMake/OrthancStoneConfiguration.cmake)

add_library(OrthancStone STATIC
  ${ORTHANC_STONE_SOURCES}
  )

#####################################################################
## Build all the sample applications
#####################################################################

include_directories(${ORTHANC_STONE_ROOT})

list(APPEND RTVIEWERDEMO_APPLICATION_SOURCES
  ${ORTHANC_STONE_ROOT}/Applications/Samples/SampleInteractor.h
  ${ORTHANC_STONE_ROOT}/Applications/Samples/SampleApplicationBase.h
  )

if (ENABLE_WASM)
  list(APPEND RTVIEWERDEMO_APPLICATION_SOURCES
    ${STONE_WASM_SOURCES}
    )
endif()

add_executable(RtViewerDemo
  main.cpp
  ${RTVIEWERDEMO_APPLICATION_SOURCES}
)
set_target_properties(RtViewerDemo PROPERTIES COMPILE_DEFINITIONS ORTHANC_STONE_SAMPLE=3)
target_include_directories(RtViewerDemo PRIVATE ${ORTHANC_STONE_ROOT})
target_link_libraries(RtViewerDemo OrthancStone)


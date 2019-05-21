# Usage (Linux):
# to build the WASM samples
# source ~/Downloads/emsdk/emsdk_env.sh && cmake -DCMAKE_TOOLCHAIN_FILE=${EMSCRIPTEN}/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_BUILD_TYPE=Release -DSTONE_SOURCES_DIR=$currentDir/../../../orthanc-stone -DORTHANC_FRAMEWORK_SOURCE=path -DORTHANC_FRAMEWORK_ROOT=$currentDir/../../../orthanc -DALLOW_DOWNLOADS=ON .. -DENABLE_WASM=ON
# to build the Qt samples

cmake_minimum_required(VERSION 2.8.3)
project(OrthancStone)

include(../../Resources/CMake/OrthancStoneParameters.cmake)

if (OPENSSL_NO_CAPIENG)
add_definitions(-DOPENSSL_NO_CAPIENG=1)
endif()


# the following block has been borrowed from orthanc/**/Compiler.cmake
if (MSVC_MULTIPLE_PROCESSES)
# "If you omit the processMax argument in the /MP option, the
# compiler obtains the number of effective processors from the
# operating system, and then creates one process per effective
# processor"
# https://blog.kitware.com/cmake-building-with-all-your-cores/
# https://docs.microsoft.com/en-us/cpp/build/reference/mp-build-with-multiple-processes
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()


#set(ENABLE_DCMTK ON)

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

# include(../../Resources/CMake/Version.cmake)

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
## Build all the sample applications
#####################################################################

include_directories(${ORTHANC_STONE_ROOT})

# files common to all samples
list(APPEND SAMPLE_APPLICATIONS_SOURCES
  ${ORTHANC_STONE_ROOT}/Applications/Samples/SampleInteractor.h
  ${ORTHANC_STONE_ROOT}/Applications/Samples/SampleApplicationBase.h
  )

if (ENABLE_QT)
  list(APPEND SAMPLE_APPLICATIONS_SOURCES
    ${ORTHANC_STONE_ROOT}/Applications/Samples/Qt/SampleQtApplicationRunner.h
    ${ORTHANC_STONE_ROOT}/Applications/Samples/Qt/SampleMainWindow.cpp
    ${ORTHANC_STONE_ROOT}/Applications/Samples/Qt/SampleMainWindowWithButtons.cpp
    )

  ORTHANC_QT_WRAP_UI(SAMPLE_APPLICATIONS_SOURCES
    ${ORTHANC_STONE_ROOT}/Applications/Samples/Qt/SampleMainWindow.ui
    ${ORTHANC_STONE_ROOT}/Applications/Samples/Qt/SampleMainWindowWithButtons.ui
    )

  ORTHANC_QT_WRAP_CPP(SAMPLE_APPLICATIONS_SOURCES
    ${ORTHANC_STONE_ROOT}/Applications/Qt/QCairoWidget.h
    ${ORTHANC_STONE_ROOT}/Applications/Qt/QStoneMainWindow.h
    ${ORTHANC_STONE_ROOT}/Applications/Samples/Qt/SampleMainWindow.h
    ${ORTHANC_STONE_ROOT}/Applications/Samples/Qt/SampleMainWindowWithButtons.h
    )
endif()

if (ENABLE_NATIVE)
  list(APPEND SAMPLE_APPLICATIONS_SOURCES
    ${ORTHANC_STONE_ROOT}/Applications/Samples/SampleMainNative.cpp
    )

elseif (ENABLE_WASM)

  list(APPEND SAMPLE_APPLICATIONS_SOURCES
    ${ORTHANC_STONE_ROOT}/Applications/Samples/SampleMainWasm.cpp
    ${STONE_WASM_SOURCES}
    )
endif()


macro(BuildSingleFileSample Target Header Sample)
  add_executable(${Target}
    ${ORTHANC_STONE_ROOT}/Applications/Samples/${Header}
    ${SAMPLE_APPLICATIONS_SOURCES}
    )
  set_target_properties(${Target} PROPERTIES COMPILE_DEFINITIONS ORTHANC_STONE_SAMPLE=${Sample})
  target_link_libraries(${Target} OrthancStone)
endmacro()

#BuildSingleFileSample(OrthancStoneEmpty EmptyApplication.h 1)
#BuildSingleFileSample(OrthancStoneTestPattern TestPatternApplication.h 2)
BuildSingleFileSample(OrthancStoneSingleFrame SingleFrameApplication.h 3)
#BuildSingleFileSample(OrthancStoneSingleVolume SingleVolumeApplication.h 4)
#BuildSingleFileSample(OrthancStoneBasicPetCtFusion 5)
#BuildSingleFileSample(OrthancStoneSynchronizedSeries 6)
#BuildSingleFileSample(OrthancStoneLayoutPetCtFusion 7)
BuildSingleFileSample(OrthancStoneSimpleViewerSingleFile SimpleViewerApplicationSingleFile.h 8)  # we keep that one just as a sample before we convert another sample to this pattern
BuildSingleFileSample(OrthancStoneSingleFrameEditor SingleFrameEditorApplication.h 9)

##### SimpleViewer sample (Qt and WASM only) #######

if (ENABLE_QT OR ENABLE_WASM)

    if (ENABLE_QT)
      list(APPEND SIMPLE_VIEWER_APPLICATION_SOURCES
        ${ORTHANC_STONE_ROOT}/Applications/Samples/SimpleViewer/Qt/SimpleViewerMainWindow.cpp
        ${ORTHANC_STONE_ROOT}/Applications/Samples/SimpleViewer/Qt/SimpleViewerMainWindow.ui
        ${ORTHANC_STONE_ROOT}/Applications/Samples/SimpleViewer/Qt/mainQt.cpp
        )

      ORTHANC_QT_WRAP_UI(SIMPLE_VIEWER_APPLICATION_SOURCES
        ${ORTHANC_STONE_ROOT}/Applications/Samples/SimpleViewer/Qt/SimpleViewerMainWindow.ui
        )

      ORTHANC_QT_WRAP_CPP(SIMPLE_VIEWER_APPLICATION_SOURCES
        ${ORTHANC_STONE_ROOT}/Applications/Samples/SimpleViewer/Qt/SimpleViewerMainWindow.h
        )

elseif (ENABLE_WASM)
        list(APPEND SIMPLE_VIEWER_APPLICATION_SOURCES
            ${ORTHANC_STONE_ROOT}/Applications/Samples/SimpleViewer/Wasm/mainWasm.cpp
            ${ORTHANC_STONE_ROOT}/Applications/Samples/SimpleViewer/Wasm/SimpleViewerWasmApplicationAdapter.cpp
            ${ORTHANC_STONE_ROOT}/Applications/Samples/SimpleViewer/Wasm/SimpleViewerWasmApplicationAdapter.h
            ${STONE_WASM_SOURCES}
          )
    endif()

    add_executable(OrthancStoneSimpleViewer
      ${ORTHANC_STONE_ROOT}/Applications/Samples/SimpleViewer/AppStatus.h
      ${ORTHANC_STONE_ROOT}/Applications/Samples/SimpleViewer/MainWidgetInteractor.cpp
      ${ORTHANC_STONE_ROOT}/Applications/Samples/SimpleViewer/MainWidgetInteractor.h
      ${ORTHANC_STONE_ROOT}/Applications/Samples/SimpleViewer/SimpleViewerApplication.cpp
      ${ORTHANC_STONE_ROOT}/Applications/Samples/SimpleViewer/SimpleViewerApplication.h
      ${ORTHANC_STONE_ROOT}/Applications/Samples/SimpleViewer/ThumbnailInteractor.cpp
      ${ORTHANC_STONE_ROOT}/Applications/Samples/SimpleViewer/ThumbnailInteractor.h
      ${SIMPLE_VIEWER_APPLICATION_SOURCES}
      )
    target_link_libraries(OrthancStoneSimpleViewer OrthancStone)
endif()

#####################################################################
## Build the unit tests
#####################################################################

if (ENABLE_NATIVE)
  add_executable(UnitTests
    ${GOOGLE_TEST_SOURCES}
    ${ORTHANC_STONE_ROOT}/UnitTestsSources/TestCommands.cpp
    ${ORTHANC_STONE_ROOT}/UnitTestsSources/TestExceptions.cpp
    ${ORTHANC_STONE_ROOT}/UnitTestsSources/TestMessageBroker.cpp
    ${ORTHANC_STONE_ROOT}/UnitTestsSources/TestStrategy.cpp
    ${ORTHANC_STONE_ROOT}/UnitTestsSources/UnitTestsMain.cpp
    )

  target_link_libraries(UnitTests OrthancStone)
endif()

#####################################################################
## Generate the documentation if Doxygen is present
#####################################################################

find_package(Doxygen)
if (DOXYGEN_FOUND)
  configure_file(
    ${ORTHANC_STONE_ROOT}/Resources/OrthancStone.doxygen
    ${CMAKE_CURRENT_BINARY_DIR}/OrthancStone.doxygen
    @ONLY)

  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/OrthancStone.doxygen
    COMMENT "Generating documentation with Doxygen" VERBATIM
    )
else()
  message("Doxygen not found. The documentation will not be built.")
endif()

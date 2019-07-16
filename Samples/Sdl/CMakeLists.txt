cmake_minimum_required(VERSION 2.8.3)

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

SET(ENABLE_SDL_CONSOLE OFF CACHE BOOL "Enable the use of the MIT-licensed SDL_Console")
SET(ENABLE_GOOGLE_TEST OFF)
SET(ENABLE_LOCALE ON)
SET(ENABLE_SDL ON)
SET(ENABLE_WEB_CLIENT ON)
SET(ORTHANC_SANDBOXED OFF)
LIST(APPEND ORTHANC_BOOST_COMPONENTS program_options)

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

#
# BasicScene
# 

add_executable(BasicScene
  BasicScene.cpp
  )

target_link_libraries(BasicScene OrthancStone)

#
# TrackerSample
# 

LIST(APPEND TRACKERSAMPLE_SOURCE "TrackerSample.cpp")
LIST(APPEND TRACKERSAMPLE_SOURCE "TrackerSampleApp.cpp")
LIST(APPEND TRACKERSAMPLE_SOURCE "TrackerSampleApp.h")

if (MSVC AND MSVC_VERSION GREATER 1700)
  LIST(APPEND TRACKERSAMPLE_SOURCE "cpp.hint")
endif()

add_executable(TrackerSample
  ${TRACKERSAMPLE_SOURCE}
  )

target_link_libraries(TrackerSample OrthancStone)

#
# Loader
# 

add_executable(Loader
  Loader.cpp
  )

target_link_libraries(Loader OrthancStone)

#
# FusionMprSdl
# 

add_executable(FusionMprSdl
  FusionMprSdl.cpp
  FusionMprSdl.h
)

target_link_libraries(FusionMprSdl OrthancStone)

#
# RadiographyEditor
#

LIST(APPEND RADIOGRAPHY_EDITOR_SOURCE "../Shared/RadiographyEditorApp.cpp")
LIST(APPEND RADIOGRAPHY_EDITOR_SOURCE "../Shared/RadiographyEditorApp.h")
LIST(APPEND RADIOGRAPHY_EDITOR_SOURCE "RadiographyEditor.cpp")

if (MSVC AND MSVC_VERSION GREATER 1700)
  LIST(APPEND RADIOGRAPHY_EDITOR_SOURCE "cpp.hint")
endif()

add_executable(RadiographyEditor
  ${RADIOGRAPHY_EDITOR_SOURCE}
  )

target_link_libraries(RadiographyEditor OrthancStone)

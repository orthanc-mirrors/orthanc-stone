cmake_minimum_required(VERSION 2.8.10)

project(RtViewerSdl)

set(ORTHANC_FRAMEWORK_SOURCE "path")
set(ORTHANC_FRAMEWORK_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../../orthanc)
set(STONE_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../)

include(${STONE_ROOT}/Resources/CMake/OrthancStoneParameters.cmake)
include(${ORTHANC_ROOT}/Resources/CMake/DownloadPackage.cmake)

SET(ENABLE_GOOGLE_TEST OFF)
SET(ENABLE_LOCALE ON)  # Necessary for text rendering
SET(ENABLE_QT OFF)
SET(ENABLE_SDL ON)
SET(ENABLE_DCMTK ON)  # <==
SET(ENABLE_OPENGL ON)  #  <==
SET(ENABLE_WEB_CLIENT ON)
SET(ORTHANC_SANDBOXED OFF)

DownloadPackage(
  "a24b8136b8f3bb93f166baf97d9328de"
  "http://orthanc.osimis.io/ThirdPartyDownloads/ubuntu-font-family-0.83.zip"
  "${CMAKE_BINARY_DIR}/ubuntu-font-family-0.83")

set(ORTHANC_STONE_APPLICATION_RESOURCES
  UBUNTU_FONT  ${CMAKE_BINARY_DIR}/ubuntu-font-family-0.83/Ubuntu-R.ttf
  )

include(${STONE_ROOT}/Resources/CMake/OrthancStoneConfiguration.cmake)
include(${STONE_ROOT}/Resources/CMake/Utilities.cmake)

include_directories(${STONE_ROOT})
include_directories(../../Common)

add_definitions(
  -DORTHANC_ENABLE_LOGGING=1
  -DORTHANC_ENABLE_LOGGING_PLUGIN=0
  -DORTHANC_ENABLE_PUGIXML=0
  -DORTHANC_DEFAULT_DICOM_ENCODING=Encoding_Latin1
  )

SortFilesInSourceGroups()

add_executable(RtViewerSdl
  RtViewerSdl.cpp
  ../SdlHelpers.h
  ../../Common/RtViewerApp.cpp
  ../../Common/RtViewerApp.h
  ../../Common/RtViewerView.cpp
  ../../Common/RtViewerView.h
  ../../Common/SampleHelpers.h
  ${ORTHANC_STONE_SOURCES}
  )

target_link_libraries(RtViewerSdl ${DCMTK_LIBRARIES})


cmake_minimum_required(VERSION 2.8.10)

project(SdlSimpleViewer)

set(ORTHANC_FRAMEWORK_SOURCE "path")
set(ORTHANC_FRAMEWORK_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../../orthanc)
set(STONE_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../)

include(${STONE_ROOT}/Resources/CMake/OrthancStoneParameters.cmake)

SET(ENABLE_GOOGLE_TEST OFF)
SET(ENABLE_LOCALE ON)  # Necessary for text rendering
SET(ENABLE_QT OFF)
SET(ENABLE_SDL ON)
SET(ENABLE_DCMTK ON)  # <==
SET(ENABLE_OPENGL ON)  #  <==
SET(ENABLE_WEB_CLIENT ON)
SET(ORTHANC_SANDBOXED OFF)

include(${STONE_ROOT}/Resources/CMake/OrthancStoneConfiguration.cmake)
include(${STONE_ROOT}/Resources/CMake/Utilities.cmake)

include_directories(${STONE_ROOT})

add_definitions(
  -DORTHANC_ENABLE_LOGGING=1
  -DORTHANC_ENABLE_LOGGING_PLUGIN=0
  -DORTHANC_ENABLE_PUGIXML=0
  -DORTHANC_DEFAULT_DICOM_ENCODING=Encoding_Latin1
  )

SortFilesInSourceGroups()

add_executable(SdlSimpleViewer
  SdlSimpleViewerApplication.h
  SdlSimpleViewer.cpp
  ${ORTHANC_STONE_SOURCES}
  )


target_link_libraries(SdlSimpleViewer ${DCMTK_LIBRARIES})


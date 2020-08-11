cmake_minimum_required(VERSION 2.8.10)

project(SdlSimpleViewer)

include(${CMAKE_SOURCE_DIR}/../../../Resources/CMake/OrthancStoneParameters.cmake)

if (ORTHANC_FRAMEWORK_SOURCE STREQUAL "system")
  set(ORTHANC_BOOST_COMPONENTS program_options)

  set(USE_SYSTEM_GOOGLE_TEST ON CACHE BOOL "Use the system version of Google Test")
  set(USE_GOOGLE_TEST_DEBIAN_PACKAGE OFF CACHE BOOL "Use the sources of Google Test shipped with libgtest-dev (Debian only)")
  mark_as_advanced(USE_GOOGLE_TEST_DEBIAN_PACKAGE)
  include(${ORTHANC_STONE_ROOT}/Resources/Orthanc/CMake/DownloadPackage.cmake)
  include(${ORTHANC_STONE_ROOT}/Resources/Orthanc/CMake/GoogleTestConfiguration.cmake)

else()
  set(ENABLE_GOOGLE_TEST ON)
  set(ENABLE_LOCALE ON)  # Necessary for text rendering
  set(ENABLE_OPENGL ON)  #  <==
  set(ENABLE_WEB_CLIENT ON)
endif()

set(ENABLE_DCMTK ON)  # <==
set(ENABLE_SDL ON)

include(${ORTHANC_STONE_ROOT}/Resources/CMake/OrthancStoneConfiguration.cmake)
include(${CMAKE_SOURCE_DIR}/../Utilities.cmake)

if (NOT ORTHANC_FRAMEWORK_SOURCE STREQUAL "system")
  # This include must be after "OrthancStoneConfiguration.cmake" to
  # have "BOOST_SOURCES_DIR" defined
  include(${CMAKE_SOURCE_DIR}/../BoostExtendedConfiguration.cmake)
endif()

SortFilesInSourceGroups()

add_executable(SdlSimpleViewer
  ../SdlHelpers.h
  ../../Common/SampleHelpers.h
  SdlSimpleViewerApplication.h
  SdlSimpleViewer.cpp
  ${ORTHANC_STONE_SOURCES}
  )

target_link_libraries(SdlSimpleViewer ${DCMTK_LIBRARIES})


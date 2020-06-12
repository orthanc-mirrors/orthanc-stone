cmake_minimum_required(VERSION 2.8.10)

project(OrthancStone)

set(ORTHANC_FRAMEWORK_SOURCE "path" CACHE STRING "select the source of Orthanc")
set(ORTHANC_FRAMEWORK_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../orthanc CACHE STRING "select the root path of Orthanc")
set(STONE_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../)

include(${STONE_ROOT}/Resources/CMake/OrthancStoneParameters.cmake)
include(${ORTHANC_ROOT}/Resources/CMake/DownloadPackage.cmake)

set(ALLOW_DOWNLOADS ON)
set(STANDALONE_BUILD ON)  # To have DCMTK_USE_EMBEDDED_DICTIONARIES defined
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  set(STATIC_BUILD ON)
endif()
set(ENABLE_GOOGLE_TEST ON)
set(ENABLE_LOCALE ON)  # Necessary for text rendering
set(ENABLE_SDL ON)
set(ENABLE_DCMTK ON)  # <==
set(ENABLE_OPENGL ON)  #  <==
set(ENABLE_WEB_CLIENT ON)
set(ORTHANC_SANDBOXED OFF)

DownloadPackage(
  "a24b8136b8f3bb93f166baf97d9328de"
  "http://orthanc.osimis.io/ThirdPartyDownloads/ubuntu-font-family-0.83.zip"
  "${CMAKE_BINARY_DIR}/ubuntu-font-family-0.83")

set(ORTHANC_STONE_APPLICATION_RESOURCES
  UBUNTU_FONT  ${CMAKE_BINARY_DIR}/ubuntu-font-family-0.83/Ubuntu-R.ttf
  )

list(APPEND ORTHANC_BOOST_COMPONENTS program_options)
include(${STONE_ROOT}/Resources/CMake/OrthancStoneConfiguration.cmake)
include(${STONE_ROOT}/Resources/CMake/BoostExtendedConfiguration.cmake)
include(${STONE_ROOT}/Resources/CMake/Utilities.cmake)

add_definitions(
  -DORTHANC_ENABLE_LOGGING=1
  -DORTHANC_ENABLE_LOGGING_PLUGIN=0
  -DORTHANC_ENABLE_PUGIXML=0
  -DORTHANC_DEFAULT_DICOM_ENCODING=Encoding_Latin1
  )

SortFilesInSourceGroups()

add_library(OrthancStone STATIC
  ${ORTHANC_STONE_SOURCES}
  )


#############################
project(RtViewerSdl)

add_executable(RtViewerSdl
  RtViewer/RtViewerSdl.cpp
  SdlHelpers.h
  ../Common/RtViewerApp.cpp
  ../Common/RtViewerApp.h
  ../Common/RtViewerView.cpp
  ../Common/RtViewerView.h
  ../Common/SampleHelpers.h
  )

target_link_libraries(RtViewerSdl OrthancStone ${DCMTK_LIBRARIES})

#############################
project(SdlSimpleViewer)

add_executable(SdlSimpleViewer
  SdlHelpers.h
  ../Common/SampleHelpers.h
  SingleFrameViewer/SdlSimpleViewerApplication.h
  SingleFrameViewer/SdlSimpleViewer.cpp
  ${BOOST_EXTENDED_SOURCES}
  )

target_link_libraries(SdlSimpleViewer OrthancStone ${DCMTK_LIBRARIES})

#############################
project(UnitTests)

add_executable(UnitTests
  ${GOOGLE_TEST_SOURCES}
  ${ORTHANC_STONE_ROOT}/UnitTestsSources/GenericToolboxTests.cpp
  ${ORTHANC_STONE_ROOT}/UnitTestsSources/ImageToolboxTests.cpp
  ${ORTHANC_STONE_ROOT}/UnitTestsSources/PixelTestPatternsTests.cpp
  ${ORTHANC_STONE_ROOT}/UnitTestsSources/TestCommands.cpp
  ${ORTHANC_STONE_ROOT}/UnitTestsSources/TestMessageBroker.cpp
  ${ORTHANC_STONE_ROOT}/UnitTestsSources/TestStrategy.cpp
  ${ORTHANC_STONE_ROOT}/UnitTestsSources/TestStructureSet.cpp
  ${ORTHANC_STONE_ROOT}/UnitTestsSources/UnitTestsMain.cpp
)

target_link_libraries(UnitTests OrthancStone)

add_custom_command(
  TARGET UnitTests
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
    "${ORTHANC_STONE_ROOT}/UnitTestsSources/72c773ac-5059f2c4-2e6a9120-4fd4bca1-45701661.json" 
    "$<TARGET_FILE_DIR:UnitTests>/72c773ac-5059f2c4-2e6a9120-4fd4bca1-45701661.json"
)

target_link_libraries(UnitTests OrthancStone ${DCMTK_LIBRARIES})

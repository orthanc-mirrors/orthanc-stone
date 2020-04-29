cmake_minimum_required(VERSION 2.8.3)
project(OrthancStone)

set(ORTHANC_FRAMEWORK_SOURCE "path")
set(ORTHANC_FRAMEWORK_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../orthanc)
set(STONE_ROOT ${CMAKE_CURRENT_LIST_DIR}/../)

include(../Resources/CMake/OrthancStoneParameters.cmake)

set(ENABLE_STONE_DEPRECATED ON)  # Need deprecated classes for these samples

if (OPENSSL_NO_CAPIENG)
add_definitions(-DOPENSSL_NO_CAPIENG=1)
endif()

set(ENABLE_SDL OFF CACHE BOOL "Target SDL Native application")
set(ENABLE_WASM OFF CACHE BOOL "Target WASM application")
set(ENABLE_UNITTESTS ON BOOL "Enable unit tests")


if (ENABLE_WASM)
  set(ORTHANC_SANDBOXED ON)
  if (ENABLE_SDL)
    message("ENABLE_SDL is only supported in native (incompatible with ENABLE_WASM)")
  endif()
  set(ENABLE_NATIVE OFF)
  set(ORTHANC_SANDBOXED OFF)
  set(ENABLE_CRYPTO_OPTIONS ON)
  set(ENABLE_GOOGLE_TEST ON)
  set(ENABLE_WEB_CLIENT ON)
elseif (ENABLE_SDL)
  set(ENABLE_NATIVE ON)
  set(ORTHANC_SANDBOXED OFF)
  set(ENABLE_CRYPTO_OPTIONS ON)
  set(ENABLE_GOOGLE_TEST ON)
  set(ENABLE_WEB_CLIENT ON)
else()
  set(ENABLE_NATIVE ON)
  set(ENABLE_OPENGL OFF)
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

include(../Resources/CMake/OrthancStoneConfiguration.cmake)

add_library(OrthancStone STATIC
  ${ORTHANC_STONE_SOURCES}
  )

include_directories(${ORTHANC_STONE_ROOT})
  
#####################################################################
## Build the unit tests
#####################################################################

if (ENABLE_UNITTESTS)
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

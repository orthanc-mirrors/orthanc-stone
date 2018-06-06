cmake_minimum_required(VERSION 2.8)
project(OrthancStone)


#####################################################################
## Configuration for Orthanc
#####################################################################

include(../../Resources/CMake/Version.cmake)

if (ORTHANC_STONE_VERSION STREQUAL "mainline")
  set(ORTHANC_FRAMEWORK_VERSION "mainline")
  set(ORTHANC_FRAMEWORK_DEFAULT_SOURCE "hg")
else()
  set(ORTHANC_FRAMEWORK_VERSION "1.3.2")
  set(ORTHANC_FRAMEWORK_DEFAULT_SOURCE "web")
endif()

set(ORTHANC_FRAMEWORK_SOURCE "${ORTHANC_FRAMEWORK_DEFAULT_SOURCE}" CACHE STRING "Source of the Orthanc source code (can be \"hg\", \"archive\", \"web\" or \"path\")")
set(ORTHANC_FRAMEWORK_ARCHIVE "" CACHE STRING "Path to the Orthanc archive, if ORTHANC_FRAMEWORK_SOURCE is \"archive\"")
set(ORTHANC_FRAMEWORK_ROOT "" CACHE STRING "Path to the Orthanc source directory, if ORTHANC_FRAMEWORK_SOURCE is \"path\"")


#####################################################################
## Build a static library containing the Orthanc Stone framework
#####################################################################

include(../../Resources/CMake/OrthancStoneParameters.cmake)

LIST(APPEND ORTHANC_BOOST_COMPONENTS program_options)

SET(ORTHANC_SANDBOXED OFF)
SET(ENABLE_CRYPTO_OPTIONS ON)
SET(ENABLE_GOOGLE_TEST ON)
SET(ENABLE_WEB_CLIENT ON)

include(../../Resources/CMake/OrthancStoneConfiguration.cmake)

add_library(OrthancStone STATIC
  ${ORTHANC_STONE_SOURCES}
  )


#####################################################################
## Build all the sample applications
#####################################################################

macro(BuildSample Target Sample)
  add_executable(${Target}
    ${ORTHANC_STONE_ROOT}/Applications/Samples/SampleMainSdl.cpp
    ${APPLICATIONS_SOURCES}
    )
  set_target_properties(${Target} PROPERTIES COMPILE_DEFINITIONS ORTHANC_STONE_SAMPLE=${Sample})
  target_link_libraries(${Target} OrthancStone)
endmacro()


# TODO - Re-enable all these samples!

BuildSample(OrthancStoneEmpty 1)
BuildSample(OrthancStoneTestPattern 2)
BuildSample(OrthancStoneSingleFrame 3)
BuildSample(OrthancStoneSingleVolume 4)
#BuildSample(OrthancStoneBasicPetCtFusion 5)
#BuildSample(OrthancStoneSynchronizedSeries 6)
#BuildSample(OrthancStoneLayoutPetCtFusion 7)


#####################################################################
## Build the unit tests
#####################################################################

add_executable(UnitTests
  ${GOOGLE_TEST_SOURCES}
  ${ORTHANC_STONE_ROOT}/UnitTestsSources/UnitTestsMain.cpp
  )

target_link_libraries(UnitTests OrthancStone)


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

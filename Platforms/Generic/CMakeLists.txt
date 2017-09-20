cmake_minimum_required(VERSION 2.8)
project(OrthancStone)


#####################################################################
## Build a static library containing the Orthanc Stone framework
#####################################################################

SET(STONE_SANDBOXED OFF)
include(../../Resources/CMake/OrthancStone.cmake)
add_library(OrthancStone STATIC ${ORTHANC_STONE_SOURCES})


#####################################################################
## Build all the sample applications
#####################################################################

macro(BuildSample Target Sample)
  add_executable(${Target}
    ${ORTHANC_STONE_DIR}/Applications/Samples/SampleMainSdl.cpp
    ${APPLICATIONS_SOURCES}
    )
  set_target_properties(${Target} PROPERTIES COMPILE_DEFINITIONS ORTHANC_STONE_SAMPLE=${Sample})
  target_link_libraries(${Target} OrthancStone)
endmacro()

# TODO - Re-enable this stuff!

#BuildSample(OrthancStoneEmpty 1)
#BuildSample(OrthancStoneTestPattern 2)
#BuildSample(OrthancStoneSingleFrame 3)
#BuildSample(OrthancStoneSingleVolume 4)
#BuildSample(OrthancStoneBasicPetCtFusion 5)
#BuildSample(OrthancStoneSynchronizedSeries 6)
#BuildSample(OrthancStoneLayoutPetCtFusion 7)


#####################################################################
## Build the unit tests
#####################################################################

add_executable(UnitTests
  ${GOOGLE_TEST_SOURCES}
  ${ORTHANC_STONE_DIR}/UnitTestsSources/UnitTestsMain.cpp
  )

target_link_libraries(UnitTests OrthancStone)


#####################################################################
## Generate the documentation if Doxygen is present
#####################################################################

find_package(Doxygen)
if (DOXYGEN_FOUND)
  configure_file(
    ${ORTHANC_STONE_DIR}/Resources/OrthancStone.doxygen
    ${CMAKE_CURRENT_BINARY_DIR}/OrthancStone.doxygen
    @ONLY)

  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/OrthancStone.doxygen
    COMMENT "Generating documentation with Doxygen" VERBATIM
    )
else()
  message("Doxygen not found. The documentation will not be built.")
endif()

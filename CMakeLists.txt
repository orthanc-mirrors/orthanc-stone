cmake_minimum_required(VERSION 2.8)
project(OrthancStone)

include(Resources/CMake/OrthancStone.cmake)

add_library(OrthancStone STATIC ${ORTHANC_STONE_SOURCES})

macro(BuildSample Target Sample)
  add_executable(${Target} Samples/SampleMainSdl.cpp)
  set_target_properties(${Target} PROPERTIES COMPILE_DEFINITIONS ORTHANC_STONE_SAMPLE=${Sample})
  target_link_libraries(${Target} OrthancStone)
endmacro()

BuildSample(OrthancStoneEmpty 1)
BuildSample(OrthancStoneTestPattern 2)
BuildSample(OrthancStoneSingleFrame 3)
BuildSample(OrthancStoneSingleVolume 4)
BuildSample(OrthancStoneBasicPetCtFusion 5)
BuildSample(OrthancStoneSynchronizedSeries 6)
BuildSample(OrthancStoneLayoutPetCtFusion 7)

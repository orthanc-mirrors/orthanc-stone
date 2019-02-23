cmake_minimum_required(VERSION 2.8)

project(testWasmIntegratedCpp)

set(testWasmIntegratedCpp_Codegen_Deps
  ${CMAKE_CURRENT_LIST_DIR}/testWasmIntegratedCpp_api.yaml 
  ${CMAKE_CURRENT_LIST_DIR}/../template.in.h
  ${CMAKE_CURRENT_LIST_DIR}/../template.in.ts
) 

add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/testWasmIntegratedCpp_generated.hpp ${CMAKE_CURRENT_BINARY_DIR}/testWasmIntegratedCpp_generated.ts
    COMMAND python3 ${CMAKE_CURRENT_LIST_DIR}/../stonegentool.py -o ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR}/../test_data/testWasmIntegratedCpp_api.yaml
    DEPENDS ${testCppHandler_Codegen_Deps}
)

add_library(testWasmIntegratedCpp main.cpp ${CMAKE_CURRENT_BINARY_DIR}/testWasmIntegratedCpp_generated.hpp ${testCppHandler_Codegen_Deps})

target_include_directories(testWasmIntegratedCpp  PUBLIC ${CMAKE_BINARY_DIR})

set_property(TARGET testWasmIntegratedCpp PROPERTY CXX_STANDARD 11)



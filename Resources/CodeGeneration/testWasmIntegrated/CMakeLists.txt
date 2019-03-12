cmake_minimum_required(VERSION 2.8)

project(testWasmIntegratedCpp)

set(WASM_FLAGS "-s WASM=1 -O0 -g0")
set(WASM_MODULE_NAME "TestWasmIntegratedModule" CACHE STRING "Name of the WebAssembly module")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WASM_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WASM_FLAGS}")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --js-library ${STONE_SOURCES_DIR}/Platforms/Wasm/WasmWebService.js --js-library ${STONE_SOURCES_DIR}/Platforms/Wasm/WasmDelayedCallExecutor.js --js-library ${STONE_SOURCES_DIR}/Platforms/Wasm/default-library.js  -s EXTRA_EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\"]'")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --js-library ${CMAKE_CURRENT_LIST_DIR}/DefaultLibrary.js -s EXTRA_EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\"]'")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s DISABLE_EXCEPTION_CATCHING=0 -s EXPORT_NAME='\"${WASM_MODULE_NAME}\"' -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=536870912 -s TOTAL_STACK=128000000")  # 512MB + resize

add_definitions(-DORTHANC_ENABLE_WASM=1)

set(testWasmIntegratedCpp_Codegen_Deps
  ${CMAKE_CURRENT_LIST_DIR}/testWasmIntegratedCpp_api.yaml 
  ${CMAKE_CURRENT_LIST_DIR}/../template.in.h.j2
  ${CMAKE_CURRENT_LIST_DIR}/../template.in.ts.j2
) 

set(jsoncppRootDir ${CMAKE_CURRENT_LIST_DIR}/jsoncpp-1.8.4)

add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/testWasmIntegratedCpp_generated.hpp ${CMAKE_CURRENT_BINARY_DIR}/testWasmIntegratedCpp_generated.ts
    COMMAND python3 ${CMAKE_CURRENT_LIST_DIR}/../stonegentool.py -o ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR}/testWasmIntegratedCpp_api.yaml
    DEPENDS ${testCppHandler_Codegen_Deps} ${CMAKE_CURRENT_LIST_DIR}/testWasmIntegratedCpp_api.yaml 
)

add_executable(testWasmIntegratedCpp
  main.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/testWasmIntegratedCpp_generated.hpp
  ${jsoncppRootDir}/jsoncpp.cpp
  ${testCppHandler_Codegen_Deps})

target_include_directories(testWasmIntegratedCpp  PUBLIC ${CMAKE_BINARY_DIR})
target_include_directories(testWasmIntegratedCpp  PUBLIC ${jsoncppRootDir})

set_property(TARGET testWasmIntegratedCpp PROPERTY CXX_STANDARD 11)



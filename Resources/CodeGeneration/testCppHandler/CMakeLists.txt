cmake_minimum_required(VERSION 2.8)

project(testCppHandler)

set(testCppHandler_Codegen_Deps
  ${CMAKE_CURRENT_LIST_DIR}/../test_data/test1.yaml 
  ${CMAKE_CURRENT_LIST_DIR}/../template.in.h.j2
) 

add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/VsolMessages_generated.hpp
    COMMAND python ${CMAKE_CURRENT_LIST_DIR}/../stonegentool.py -o ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR}/../test_data/test1.yaml
    DEPENDS ${testCppHandler_Codegen_Deps}
)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup()

add_executable(testCppHandler main.cpp ${CMAKE_CURRENT_BINARY_DIR}/VsolMessages_generated.hpp ${testCppHandler_Codegen_Deps})

target_include_directories(testCppHandler PUBLIC ${CMAKE_BINARY_DIR})

conan_target_link_libraries(testCppHandler)

set_property(TARGET testCppHandler PROPERTY CXX_STANDARD 17)

install(TARGETS testCppHandler DESTINATION bin)


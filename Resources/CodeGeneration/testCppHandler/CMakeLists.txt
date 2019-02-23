cmake_minimum_required(VERSION 2.8)

project(testCppHandler)

add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/VsolMessages_generated.hpp
    COMMAND python ${CMAKE_CURRENT_LIST_DIR}/../stonegentool.py -o ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR}/../test_data/test1.yaml
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/../test_data/test1.yaml
)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup()

add_executable(testCppHandler main.cpp ${CMAKE_CURRENT_BINARY_DIR}/VsolMessages_generated.hpp)

target_include_directories(testCppHandler PUBLIC ${CMAKE_BINARY_DIR})

conan_target_link_libraries(testCppHandler)

set_property(TARGET testCppHandler PROPERTY CXX_STANDARD 17)

install(TARGETS testCppHandler DESTINATION bin)


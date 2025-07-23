cmake_minimum_required(VERSION 3.21)
project(mytest LANGUAGES CXX)

set(public_headers
    include/mytest.hpp
    include/mytest_internal.hpp)
set(sources
    ${public_headers}
    src/mytest.cpp
    src/mytest_main.cpp)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})

add_library(mytest STATIC)
target_sources(mytest PRIVATE ${sources})

target_include_directories(mytest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

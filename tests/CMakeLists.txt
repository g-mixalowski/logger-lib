cmake_minimum_required(VERSION 3.21)
project(loggerlib-tests)

add_subdirectory(mytest)

if(PROJECT_IS_TOP_LEVEL)
    enable_testing()
    find_package(loggerlib REQUIRED)
endif()

set(sources 
    tests.cpp)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})

add_executable(loggerlib-tests)
target_sources(loggerlib-tests PRIVATE ${sources})

target_link_libraries(loggerlib-tests
    PRIVATE
        loggerlib::loggerlib
        mytest)
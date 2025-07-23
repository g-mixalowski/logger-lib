cmake_minimum_required(VERSION 3.21)
project(logger-test-app LANGUAGES CXX)

if (PROJECT_IS_TOP_LEVEL)
    find_package(loggerlib REQUIRED)
endif()

set(sources main.cpp)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})

add_executable(logger-test-app)
target_sources(logger-test-app PRIVATE ${sources})
target_link_libraries(logger-test-app PRIVATE loggerlib::loggerlib)

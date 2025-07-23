cmake_minimum_required(VERSION 3.21)
project(logger-stats-app LANGUAGES CXX)

if (PROJECT_IS_TOP_LEVEL)
    find_package(loggerlib REQUIRED)
endif()

set(sources main.cpp)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})

add_executable(logger-stats-app)
target_sources(logger-stats-app PRIVATE ${sources})
target_link_libraries(logger-stats-app PRIVATE loggerlib::loggerlib)

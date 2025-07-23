cmake_minimum_required(VERSION 3.21)
project(loggerlib
    VERSION 1.0.0
    DESCRIPTION "Lib for logging messages into file or socket"
    LANGUAGES CXX)

################################
# general settings and options #
################################

include(GNUInstallDirs)
include(cmake/utils.cmake)

# LOGGERLIB_SHARED_LIBS determines static/shared build when defined
option(LOGGERLIB_BUILD_TESTS "Build loggerlib tests" OFF)
option(LOGGERLIB_BUILD_EXAMPLES "Build examples" OFF)
option(LOGGERLIB_INSTALL "Generate target for installing loggerlib" ${PROJECT_IS_TOP_LEVEL})
set_if_undefined(LOGGERLIB_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/loggerlib-${PROJECT_VERSION}" CACHE STRING
    "Install path for loggerlib package-related CMake files")

if(DEFINED LOGGERLIB_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ${LOGGERLIB_SHARED_LIBS})
endif()

if(NOT DEFINED CMAKE_BUILD_TYPE AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set_if_undefined(CMAKE_CXX_VISIBILITY_PRESET hidden)
set_if_undefined(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

add_library(loggerlib) # initialized below
add_library(loggerlib::loggerlib ALIAS loggerlib)

#####################
# loggerlib sources #
#####################

# generate export_*.h to prevent names conflict
include(GenerateExportHeader)
set(export_file_name "export_shared.hpp")

if (NOT BUILD_SHARED_LIBS)
    set(export_file_name "export_static.hpp")
endif()

generate_export_header(loggerlib EXPORT_FILE_NAME include/loggerlib/${export_file_name})

set(public_headers
    include/loggerlib/export.hpp
    include/loggerlib/logger.hpp)
set(sources
    ${public_headers}
    src/logger.cpp)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})

list(APPEND public_headers
    "${CMAKE_CURRENT_BINARY_DIR}/include/loggerlib/${export_file_name}")
list(APPEND sources
    "${CMAKE_CURRENT_BINARY_DIR}/include/loggerlib/${export_file_name}")

####################
# loggerlib target #
####################

include(CMakePackageConfigHelpers)

target_sources(loggerlib PRIVATE ${sources})
target_compile_definitions(loggerlib
    PUBLIC
        "$<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:LOGGERLIB_STATIC_DEFINE>")

target_include_directories(loggerlib
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>")

set_target_properties(loggerlib PROPERTIES
    SOVERSION ${PROJECT_VERSION_MAJOR}
    VERSION ${PROJECT_VERSION})

# install target
if(LOGGERLIB_INSTALL AND NOT CMAKE_SKIP_INSTALL_RULES)
    configure_package_config_file(cmake/loggerlib-config.cmake.in loggerlib-config.cmake
        INSTALL_DESTINATION "${LOGGERLIB_INSTALL_CMAKEDIR}")

    write_basic_package_version_file(loggerlib-config-version.cmake
        COMPATIBILITY SameMajorVersion)

    install(TARGETS loggerlib EXPORT loggerlib_export
        RUNTIME COMPONENT loggerlib
        LIBRARY COMPONENT loggerlib NAMELINK_COMPONENT loggerlib-dev
        ARCHIVE COMPONENT loggerlib-dev
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(DIRECTORY include/
        TYPE INCLUDE
        COMPONENT loggerlib-dev)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/loggerlib/${export_file_name}"
        COMPONENT loggerlib-dev
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/loggerlib")

    set(targets_file "loggerlib-shared-targets.cmake")

    if(NOT BUILD_SHARED_LIBS)
        set(targets_file "loggerlib-static-targets.cmake")
    endif()

    install(EXPORT loggerlib_export
        COMPONENT loggerlib-dev
        FILE "${targets_file}"
        DESTINATION "${LOGGERLIB_INSTALL_CMAKEDIR}"
        NAMESPACE loggerlib::)

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/loggerlib-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/loggerlib-config-version.cmake"
        COMPONENT loggerlib-dev
        DESTINATION "${LOGGERLIB_INSTALL_CMAKEDIR}")

    if(MSVC)
        set(pdb_file "")
        set(pdb_file_destination "")

        if(BUILD_SHARED_LIBS)
            set(pdb_file "$<TARGET_PDB_FILE:loggerlib>")
            set(pdb_file_destination "${CMAKE_INSTALL_BINDIR}")
        else()
            # TARGET_PDB_FILE does not work for pdb file generated for static library build, determining it manually
            set(pdb_file "$<TARGET_FILE_DIR:loggerlib>/$<TARGET_FILE_PREFIX:loggerlib>$<TARGET_FILE_BASE_NAME:loggerlib>.pdb")
            set(pdb_file_destination "${CMAKE_INSTALL_LIBDIR}")
        endif()

        install(FILES "${pdb_file}"
            COMPONENT loggerlib-dev
            CONFIGURATIONS Debug RelWithDebInfo
            DESTINATION "${pdb_file_destination}"
            OPTIONAL)
    endif()
endif()

#################
# other targets #
#################

# tests target
if(LOGGERLIB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# examples target
if(LOGGERLIB_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
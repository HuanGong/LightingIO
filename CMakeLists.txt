cmake_minimum_required(VERSION 2.8)

project(LightingIO)

# for generate ycm complete database
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CONFIG_DIR ${CMAKE_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CONFIG_DIR}/Modules)
MESSAGE(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")

include(${CMAKE_CONFIG_DIR}/global_config.cmake)
include(${CMAKE_CONFIG_DIR}/include_path.cmake)
include(${CMAKE_CONFIG_DIR}/package_deps.cmake)
#include(${CMAKE_CONFIG_DIR}/googletest.cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# add a default linked path to build/lib
LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/lib)

ADD_SUBDIRECTORY(thirdparty)
ADD_SUBDIRECTORY(base)
ADD_SUBDIRECTORY(net)
#ADD_SUBDIRECTORY(app)
ADD_SUBDIRECTORY(example)
ADD_SUBDIRECTORY(components)


MESSAGE(STATUS "This is BINARY dir " ${LightingIO_BINARY_DIR})
MESSAGE(STATUS "This is SOURCE dir " ${LightingIO_SOURCE_DIR})
MESSAGE(STATUS "This is CMAKE lib output dir " ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
MESSAGE(STATUS "This is CMAKE binary output dir " ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

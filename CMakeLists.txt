cmake_minimum_required(VERSION 2.8)
add_definitions("-Wall -lpthread -g")
project(LightingIO)

set(CMAKE_CONFIG_DIR ${CMAKE_SOURCE_DIR}/cmake_config)

include(${CMAKE_CONFIG_DIR}/include_path.cmake)
include(${CMAKE_CONFIG_DIR}/package_deps.cmake)
#include(${CMAKE_CONFIG_DIR}/googletest.cmake)

#set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "" FORCE)
#SET(CMAKE_CXX_FLAGS_RELEASE "-O2")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# add a default linked path to build/lib
LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/lib)


ADD_SUBDIRECTORY(thirdparty)
ADD_SUBDIRECTORY(base)
ADD_SUBDIRECTORY(net)
#ADD_SUBDIRECTORY(lighting)
ADD_SUBDIRECTORY(example)
ADD_SUBDIRECTORY(app)


MESSAGE(STATUS "This is BINARY dir " ${LightingIO_BINARY_DIR})
MESSAGE(STATUS "This is SOURCE dir " ${LightingIO_SOURCE_DIR})
MESSAGE(STATUS "This is CMAKE lib output dir " ${LIBRARY_OUTPUT_PATH})
MESSAGE(STATUS "This is CMAKE binary output dir " ${EXECUTABLE_OUTPUT_PATH})

cmake_minimum_required(VERSION 2.8)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

project(LightingIO)


# set default LIBRARY_OUTPUT_PATH to build/lib
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
# set default EXECUTABLE_OUTPUT_PATH to build/bin
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)


# add a default linked path to build/lib
LINK_DIRECTORIES(
  ${CMAKE_BINARY_DIR}/lib
  ${CMAKE_BINARY_DIR}/thirdparty/libevent/lib
)

# top level include dir
include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/base
  ${CMAKE_SOURCE_DIR}/thirdparty
  ${CMAKE_BINARY_DIR}/thirdparty
  #glog
  ${CMAKE_BINARY_DIR}/thirdparty/glog
  ${CMAKE_SOURCE_DIR}/thirdparty/glog/src
  #libevent
  ${CMAKE_BINARY_DIR}/thirdparty/libevent/include
  ${CMAKE_SOURCE_DIR}/thirdparty/libevent/include
  #coro
  ${CMAKE_BINARY_DIR}/thirdparty/libcoro
)

ADD_SUBDIRECTORY(thirdparty)
ADD_SUBDIRECTORY(base)
ADD_SUBDIRECTORY(lighting)
#ADD_SUBDIRECTORY(net)

ADD_SUBDIRECTORY(example)



MESSAGE(STATUS "This is BINARY dir " ${LightingIO_BINARY_DIR})
MESSAGE(STATUS "This is SOURCE dir " ${LightingIO_SOURCE_DIR})
MESSAGE(STATUS "This is CMAKE lib output dir " ${LIBRARY_OUTPUT_PATH})
MESSAGE(STATUS "This is CMAKE binary output dir " ${EXECUTABLE_OUTPUT_PATH})

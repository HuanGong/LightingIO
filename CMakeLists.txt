cmake_minimum_required(VERSION 2.6)

project(LightingIO)

LINK_DIRECTORIES(${LightingIO_SOURCE_DIR}/lib)

ADD_SUBDIRECTORY(base)
ADD_SUBDIRECTORY(thirdparty)
ADD_SUBDIRECTORY(thirdparty/libevent)

MESSAGE(STATUS "This is BINARY dir " ${LightingIO_BINARY_DIR})
MESSAGE(STATUS "This is SOURCE dir " ${LightingIO_SOURCE_DIR})
MESSAGE(STATUS "This is CMAKE lib output dir " ${LIBRARY_OUTPUT_PATH})
MESSAGE(STATUS "This is CMAKE binary output dir " ${EXECUTABLE_OUTPUT_PATH})

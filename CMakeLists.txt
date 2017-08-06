cmake_minimum_required(VERSION 2.6)

project(LightingIO)


# set default EXECUTABLE_OUTPUT_PATH to build/bin
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

#
# set default LIBRARY_OUTPUT_PATH to build/lib
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# add a default linked path to build/lib
LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH}/lib)

ADD_SUBDIRECTORY(base)
ADD_SUBDIRECTORY(ev_io)
ADD_SUBDIRECTORY(thirdparty)
ADD_SUBDIRECTORY(thirdparty/libevent)

MESSAGE(STATUS "This is BINARY dir " ${LightingIO_BINARY_DIR})
MESSAGE(STATUS "This is SOURCE dir " ${LightingIO_SOURCE_DIR})
MESSAGE(STATUS "This is CMAKE lib output dir " ${LIBRARY_OUTPUT_PATH})
MESSAGE(STATUS "This is CMAKE binary output dir " ${EXECUTABLE_OUTPUT_PATH})

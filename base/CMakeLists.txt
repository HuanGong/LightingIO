
SET(CMAKE_C_COMPILER g++)
#add_compile_options(-std=c++11)
ADD_DEFINITIONS(-std=c++11)

set(LIBEVENT_PATH ${LightingIO_SOURCE_DIR}/thirdparty/libevent)
set(LIBEVENT_OUT_PATH ${CMAKE_BINARY_DIR}/thirdparty/libevent)

include_directories(${LIBEVENT_PATH}/include)
include_directories(${LIBEVENT_OUT_PATH}/include)

# libevent.so path
LINK_DIRECTORIES(${LIBEVENT_OUT_PATH}/lib)

include(ExternalProject)
ExternalProject_Add(concurrentqueue
  CONFIGURE_COMMAND ""
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/thirdparty/concurrentqueue
  UPDATE_COMMAND ""
  BUILD_COMMAND ${MAKE}
  # Specify build dir location
  BINARY_DIR ${PROJECT_SOURCE_DIR}/thirdparty/concurrentqueue/build
  #BUILD_IN_SOURCE 0
  INSTALL_COMMAND ""
)

SET(BASE_SOURCES
    utils/ns_convertor.cc
    message_loop/message_loop.cc)

ADD_LIBRARY(base SHARED ${BASE_SOURCES})
TARGET_LINK_LIBRARIES(base -levent)


#test code
SET(BASE_UNITTESTS main.c)

ADD_EXECUTABLE(base_unittest ${BASE_UNITTESTS})
TARGET_LINK_LIBRARIES(base_unittest base)

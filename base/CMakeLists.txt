
SET(CMAKE_C_COMPILER g++)
#add_compile_options(-std=c++11)
ADD_DEFINITIONS(-std=c++11)

#set(LIBEVENT_PATH ${LightingIO_SOURCE_DIR}/thirdparty/libevent)
#set(LIBEVENT_OUT_PATH ${CMAKE_BINARY_DIR}/thirdparty/libevent)

#include_directories(${LIBEVENT_PATH}/include)
#include_directories(${LIBEVENT_OUT_PATH}/include)

include_directories(
  ${CMAKE_BINARY_DIR}/thirdparty

  ${CMAKE_BINARY_DIR}/thirdparty/glog
  ${CMAKE_SOURCE_DIR}/thirdparty/glog/src

  ${CMAKE_BINARY_DIR}/thirdparty/libevent/include
  ${CMAKE_SOURCE_DIR}/thirdparty/libevent/include
  )

LINK_DIRECTORIES(
  # libevent.so path
  ${CMAKE_BINARY_DIR}/thirdparty/libevent/lib
  )

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
  time_utils.cc
  utils/ns_convertor.cc
  message_loop/message_loop.cc)

#ADD_LIBRARY(base STATIC ${BASE_SOURCES})
ADD_LIBRARY(base SHARED ${BASE_SOURCES})
TARGET_LINK_LIBRARIES(base
  libglog.a
  -levent
  -lpthread
  )


#test code
SET(BASE_UNITTESTS main.c)

ADD_EXECUTABLE(base_unittest ${BASE_UNITTESTS})
TARGET_LINK_LIBRARIES(base_unittest
  libbase.a)

#messageloop test
SET(MessageLoopTestSRC
    message_loop/messageloop_test.cc)
ADD_EXECUTABLE(messageloop_test ${MessageLoopTestSRC})
TARGET_LINK_LIBRARIES(messageloop_test base)


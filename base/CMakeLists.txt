
SET(CMAKE_C_COMPILER g++)
#add_compile_options(-std=c++11)
ADD_DEFINITIONS(-std=c++11)

include_directories(
  ${CMAKE_BINARY_DIR}/thirdparty

  ${CMAKE_BINARY_DIR}/thirdparty/glog
  ${CMAKE_SOURCE_DIR}/thirdparty/glog/src

  ${CMAKE_BINARY_DIR}/thirdparty/libevent/include
  ${CMAKE_SOURCE_DIR}/thirdparty/libevent/include

  ${CMAKE_BINARY_DIR}/thirdparty/gflags/include

  ${CMAKE_BINARY_DIR}/thirdparty/libcoro
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

ADD_LIBRARY(base_static STATIC ${BASE_SOURCES})
add_dependencies(base_static
  glog
  event_static
  gflags_nothreads_static
  )
TARGET_LINK_LIBRARIES(base_static
  libglog.a
  libgflags_nothreads.a
  -levent
  -lpthread
  )

ADD_LIBRARY(base_shared SHARED ${BASE_SOURCES})
add_dependencies(base_shared
  glog
  event_shared
  gflags_nothreads_static
  )
TARGET_LINK_LIBRARIES(base_shared
  #-lglog
  -levent
  -lpthread
  )



#test code
SET(BASE_UNITTESTS main.c)
ADD_EXECUTABLE(base_unittest ${BASE_UNITTESTS})
add_dependencies(base_unittest base_static)

#messageloop test
SET(MessageLoopTestSRC
    message_loop/messageloop_test.cc)
ADD_EXECUTABLE(messageloop_test ${MessageLoopTestSRC})
TARGET_LINK_LIBRARIES(messageloop_test base_static)


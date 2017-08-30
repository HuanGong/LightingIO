SET(CMAKE_C_COMPILER g++)
#add_compile_options(-std=c++11)
ADD_DEFINITIONS(-std=c++11)
ADD_DEFINITIONS(-fPIC)


# libevent.so path
LINK_DIRECTORIES(
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
  message_loop/message_loop.cc
  coroutine/coroutine.cc
  coroutine/coroutine_scheduler.cc
)

ADD_LIBRARY(base_static STATIC ${BASE_SOURCES})
add_dependencies(base_static
  coro
  glog
  event_static
)
TARGET_LINK_LIBRARIES(base_static
  libglog.a
  libevent.a
  libcoro.a
  -lpthread
)

ADD_LIBRARY(base_shared SHARED ${BASE_SOURCES})
add_dependencies(base_shared
  coro
  glog
  event_shared
)
TARGET_LINK_LIBRARIES(base_shared
  libcoro.a
  libglog.a
  -lglog
  -levent
  -lpthread
)

#test code
SET(BASE_UNITTESTS main.c)
ADD_EXECUTABLE(base_unittest ${BASE_UNITTESTS})
add_dependencies(base_unittest base_static)

#messageloop test
ADD_EXECUTABLE(messageloop_test
  message_loop/messageloop_test.cc
)
TARGET_LINK_LIBRARIES(messageloop_test
  gflags
  base_static
)


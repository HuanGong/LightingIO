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

ADD_LIBRARY(base_shared SHARED ${BASE_SOURCES})
add_dependencies(base_shared
  glog
  coro
  event_shared
)
TARGET_LINK_LIBRARIES(base_shared
  coro
  -levent
  -lpthread
)

#messageloop test
ADD_EXECUTABLE(messageloop_test
  message_loop/messageloop_test.cc
)
TARGET_LINK_LIBRARIES(messageloop_test
  coro
  glog
  base_shared
)


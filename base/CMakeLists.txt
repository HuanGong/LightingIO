SET(CMAKE_C_COMPILER g++)

#add_compile_options(-std=c++11)
ADD_DEFINITIONS(-std=c++1y)
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
  time/timestamp.cc
  time/time_utils.cc
  utils/ns_convertor.cc
  #messageloop
  message_loop/message_loop.cc
  #coroutine
  coroutine/coroutine.cc
  coroutine/coroutine_scheduler.cc
  #new eventloop
  event_loop/fd_event.cc
  event_loop/timer_event.cc
  event_loop/event_pump.cc
  event_loop/linux_signal.cpp
  event_loop/file_util_linux.cc
  event_loop/io_multiplexer.cc
  event_loop/io_multiplexer_epoll.cc
  event_loop/timer_task_queue.cc
  event_loop/msg_event_loop.cc
)

ADD_LIBRARY(base_shared SHARED
  ${BASE_SOURCES}
)

add_dependencies(base_shared
  glog
  coro
  event_shared
)
TARGET_LINK_LIBRARIES(base_shared
  coro
  glog
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

ADD_EXECUTABLE(eventloop_test
  event_loop/test/run_loop_test.cc
)
TARGET_LINK_LIBRARIES(eventloop_test
  glog
  base_shared
)


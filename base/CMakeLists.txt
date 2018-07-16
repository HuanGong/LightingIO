SET(CMAKE_C_COMPILER g++)

#add_compile_options(-std=c++11)
ADD_DEFINITIONS(-std=c++1y)
ADD_DEFINITIONS(-fPIC)

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
  #coroutine
  coroutine/coroutine.cc
  coroutine/coroutine_scheduler.cc
  #new eventloop
  message_loop/fd_event.cc
  message_loop/timer_event.cc
  message_loop/event_pump.cc
  message_loop/linux_signal.cpp
  message_loop/file_util_linux.cc
  message_loop/io_multiplexer.cc
  message_loop/io_multiplexer_epoll.cc
  message_loop/timer_task_queue.cc
  message_loop/message_loop.cc
  #spin lock
  spin_lock.cc
  #memory
  memory/lazy_instance.cc
  # gzip compression utils
  compression_utils/gzip_utils.cc
)

SET(LTBASE_DEPENDENCY
  coro
)

SET(LTBASEEXECUTE_LINKLIBS
  ltbase
  ${GLOG_LIBRARY}
  ${ZLIB_LIBRARIES}
  coro
  gflags
  unwind
  pthread
)

ADD_LIBRARY(ltbase STATIC
  ${BASE_SOURCES}
)
add_dependencies(ltbase
  ${LTBASE_DEPENDENCY}
)

ADD_EXECUTABLE(eventloop_test
  message_loop/test/run_loop_test.cc
)
TARGET_LINK_LIBRARIES(eventloop_test
  ${LTBASEEXECUTE_LINKLIBS}
)

ADD_EXECUTABLE(eventloop_benchmark
  message_loop/test/benchmark.cc
)
TARGET_LINK_LIBRARIES(eventloop_benchmark
  ${LTBASEEXECUTE_LINKLIBS}
)

ADD_EXECUTABLE(coro2_test
  coroutine/test_main.cc
)
TARGET_LINK_LIBRARIES(coro2_test
  ${LTBASEEXECUTE_LINKLIBS}
)


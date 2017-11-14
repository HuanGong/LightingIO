include_directories(${LightingIO_SOURCE_DIR})

include_directories(${LIBEVENT_PATH}/include)
include_directories(${LIBEVENT_OUT_PATH}/include)

SET(CMAKE_C_COMPILER g++)
ADD_DEFINITIONS(-std=c++11)

# client side
ADD_LIBRARY(ltnet SHARED
  event_pump.cc
  fd_event.cc
  socket_utils.cc
  linux_signal.cpp
  file_util_linux.cc
  io_multiplexer.cc
  io_multiplexer_epoll.cc
)

TARGET_LINK_LIBRARIES(ltnet
  -lglog
  -lbase_shared
  -lpthread
)

add_dependencies(ltnet
  glog
  base_shared
)

ADD_EXECUTABLE(runloop_test
  test/run_loop_test.cc
)
TARGET_LINK_LIBRARIES(runloop_test
  ltnet
  #-lglog
  #-lbase_shared
  #-lpthread
)



include_directories(${LightingIO_SOURCE_DIR})

include_directories(${LIBEVENT_PATH}/include)
include_directories(${LIBEVENT_OUT_PATH}/include)

SET(CMAKE_C_COMPILER g++)
ADD_DEFINITIONS(-std=c++11)

# client side
ADD_LIBRARY(ltnet SHARED
  fd_event.cc
  socket_utils.cc
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


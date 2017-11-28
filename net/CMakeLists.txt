include_directories(${LightingIO_SOURCE_DIR})

include_directories(${LIBEVENT_PATH}/include)
include_directories(${LIBEVENT_OUT_PATH}/include)

SET(CMAKE_C_COMPILER g++)
ADD_DEFINITIONS(-std=c++11)

ADD_LIBRARY(ltnet STATIC
  inet_address.cc
  socket_utils.cc
  service_acceptor.cc
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

ADD_EXECUTABLE(acceptor_test
  test/acceptor_test.cc
)
TARGET_LINK_LIBRARIES(acceptor_test
  -lltnet
  -lglog
  -lbase_shared
  -lpthread
)

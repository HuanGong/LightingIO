include_directories(${LightingIO_SOURCE_DIR})

include_directories(${LIBEVENT_PATH}/include)
include_directories(${LIBEVENT_OUT_PATH}/include)

SET(CMAKE_C_COMPILER g++)
ADD_DEFINITIONS(-std=c++11)

ADD_LIBRARY(ltnet STATIC
  server.cc
  io_buffer.cc
  io_service.cc
  tcp_channel.cc
  inet_address.cc
  socket_utils.cc
  service_acceptor.cc
  url_string_utils.cc
  protocol/proto_message.cc
  protocol/proto_service.cc
  protocol/proto_service_factory.cc
  protocol/line/line_message.cc
  protocol/line/line_proto_service.cc
  protocol/http/http_message.cc
  protocol/http/http_proto_service.cc
  #dispatcher
  dispatcher/coro_dispatcher.cc
  dispatcher/workload_dispatcher.cc
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

ADD_EXECUTABLE(reuseport_test
  test/reuseport_test.cc
)
TARGET_LINK_LIBRARIES(reuseport_test
  -lltnet
  -lglog
  -lbase_shared
  -lpthread
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

ADD_EXECUTABLE(ioservice_test
  test/io_service_test.cc
)
TARGET_LINK_LIBRARIES(ioservice_test
  -lltnet
  -lglog
  -lbase_shared
  -lpthread
)

ADD_EXECUTABLE(common_server
  test/common_server_test.cc
)
TARGET_LINK_LIBRARIES(common_server
  -lltnet
  -lglog
  -lbase_shared
  -lpthread
)

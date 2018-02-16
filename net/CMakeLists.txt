SET(CMAKE_C_COMPILER g++)
ADD_DEFINITIONS(-std=c++11)
ADD_DEFINITIONS(-fPIC)

set(LTNET_SOURCE
  server.cc
  io_buffer.cc
  io_service.cc
  tcp_channel.cc
  inet_address.cc
  socket_utils.cc
  service_acceptor.cc
  url_string_utils.cc
  #protocol
  protocol/proto_message.cc
  protocol/proto_service.cc
  protocol/proto_service_factory.cc
  protocol/line/line_message.cc
  protocol/line/line_proto_service.cc
  protocol/http/http_request.cc
  protocol/http/http_response.cc
  protocol/http/http_constants.cc
  protocol/http/parser_context.cc
  protocol/http/http_proto_service.cc
  protocol/raw/raw_message.cc
  protocol/raw/raw_proto_service.cc
  #dispatcher
  dispatcher/coro_dispatcher.cc
  dispatcher/workload_dispatcher.cc

  #clients source
  clients/client_channel.cc
  clients/client_connector.cc
  clients/requests_keeper.cc
  clients/client_router.cc
)

#ADD_LIBRARY(ltnet STATIC
ADD_LIBRARY(ltnet SHARED
  ${LTNET_SOURCE}
)

TARGET_LINK_LIBRARIES(ltnet
  -lglog
  -lgflags
  -lhttp_parser
  -lbase_shared
  -lpthread
)
add_dependencies(ltnet
  http_parser
  base_shared
)

ADD_EXECUTABLE(reuseport_test
  test/reuseport_test.cc
)
add_dependencies(reuseport_test
  ltnet
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
add_dependencies(acceptor_test
  ltnet
)
TARGET_LINK_LIBRARIES(acceptor_test
  -lltnet
  -lglog
  -lgflags
  -lbase_shared
  -lpthread
)

ADD_EXECUTABLE(ioservice_test
  test/io_service_test.cc
)
add_dependencies(ioservice_test
  ltnet
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
add_dependencies(common_server
  ltnet
)
TARGET_LINK_LIBRARIES(common_server
  -ltcmalloc
  -lltnet
  -lglog
  -lgflags
  -lbase_shared
  -lpthread
)

ADD_EXECUTABLE(client_test
  test/client_test.cc
)
add_dependencies(client_test
  ltnet
)
TARGET_LINK_LIBRARIES(client_test
  -lltnet
  -lglog
  -lgflags
  -lbase_shared
  -lpthread
)

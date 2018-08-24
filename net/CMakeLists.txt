SET(CMAKE_C_COMPILER g++)
ADD_DEFINITIONS(-std=c++11)
ADD_DEFINITIONS(-fPIC)

set(LTNET_SOURCE
  #server.cc
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
  protocol/redis/redis_request.cc
  protocol/redis/redis_response.cc
  protocol/redis/resp_service.cc
  #dispatcher
  dispatcher/coro_dispatcher.cc
  dispatcher/workload_dispatcher.cc

  #server
  server/raw_server/raw_server.cc
  server/http_server/http_server.cc

  #clients source
  #clients/client_channel.cc
  clients/client_connector.cc
  #clients/requests_keeper.cc
  clients/client_router.cc
  clients/queued_channel.cc
  clients/async_channel.cc
)

SET(LTNETEXECUTE_LINKLIBS
  ${Tcmalloc_LIBRARIES}
  ltnet
  ltbase
  http_parser
  ${GLOG_LIBRARY}
  ${ZLIB_LIBRARIES}
  coro
  gflags
  unwind
  pthread
)

SET(LTNET_DEPENDENCY
  ltbase
  http_parser
)

ADD_LIBRARY(ltnet STATIC
  ${LTNET_SOURCE}
)

# 指定静态库的输出名称#set_target_properties(ltnet_static PROPERTIES OUTPUT_NAME "ltnet")
# 使动态库和静态库同时存在
#set_target_properties(ltnet PROPERTIES CLEAN_DIRECT_OUTPUT 1)
#set_target_properties(ltnet_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

add_dependencies(ltnet
  ${LTNET_DEPENDENCY}
)

ADD_EXECUTABLE(reuseport_test
  test/reuseport_test.cc
)
TARGET_LINK_LIBRARIES(reuseport_test
  ${LTNETEXECUTE_LINKLIBS}
)

ADD_EXECUTABLE(acceptor_test
  test/acceptor_test.cc
)
TARGET_LINK_LIBRARIES(acceptor_test
  ${LTNETEXECUTE_LINKLIBS}
)

ADD_EXECUTABLE(ioservice_test
  test/io_service_test.cc
)
TARGET_LINK_LIBRARIES(ioservice_test
  ${LTNETEXECUTE_LINKLIBS}
)

ADD_EXECUTABLE(client_test
  test/client_test.cc
)
TARGET_LINK_LIBRARIES(client_test
  ${LTNETEXECUTE_LINKLIBS}
)

ADD_EXECUTABLE(new_server_model
  test/new_server_model.cc
)
TARGET_LINK_LIBRARIES(new_server_model
  ${LTNETEXECUTE_LINKLIBS}
)

ADD_DEFINITIONS(-std=c++11)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
  clients/client_channel.cc
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
  ${GLOG_LIBRARY}
  ${ZLIB_LIBRARIES}
  3rd
  coro
  pthread
)

SET(LTNET_DEPENDENCY ltbase)

ADD_LIBRARY(ltnet STATIC ${LTNET_SOURCE})
ADD_LIBRARY(ltnet_shared SHARED ${LTNET_SOURCE})

set_target_properties(ltnet_shared PROPERTIES OUTPUT_NAME "ltnet")
set_target_properties(ltnet PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(ltnet_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)

add_dependencies(ltnet
  ${LTNET_DEPENDENCY}
)

link_libraries(
  ${LTNETEXECUTE_LINKLIBS}
)

ADD_SUBDIRECTORY(unittests)

ADD_EXECUTABLE(reuseport_test
  test/reuseport_test.cc
)
ADD_EXECUTABLE(acceptor_test
  test/acceptor_test.cc
)
ADD_EXECUTABLE(ioservice_test
  test/io_service_test.cc
)
ADD_EXECUTABLE(client_test
  test/client_test.cc
)
ADD_EXECUTABLE(new_server_model
  test/new_server_model.cc
)
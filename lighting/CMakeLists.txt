set(LIBEVENT_PATH ${LightingIO_SOURCE_DIR}/thirdparty/libevent)
set(LIBEVENT_OUT_PATH ${CMAKE_BINARY_DIR}/thirdparty/libevent)

include_directories(${LightingIO_SOURCE_DIR})

include_directories(${LIBEVENT_PATH}/include)
include_directories(${LIBEVENT_OUT_PATH}/include)

SET(CMAKE_C_COMPILER g++)
ADD_DEFINITIONS(-std=c++11)

ADD_EXECUTABLE(lighting_srv
  test_http_server_main.cc
)

TARGET_LINK_LIBRARIES(lighting_srv
  liblightingio.a
  -lglog
  -lbase_shared
  -levent
  -lpthread
)
add_dependencies(lighting_srv
  glog
  lightingio
  base_shared
)

# client side
ADD_LIBRARY(lightingio STATIC
  request/url_request.cc
  request/url_response.cc
  client/httpchannel_libevent.cc
  io_service.cc
  http_server.cc
)

TARGET_LINK_LIBRARIES(lightingio
  -lglog
  -levent
  -lbase_shared
  -lpthread
)
add_dependencies(lightingio
  glog
  base_shared
)

# client tesst code
ADD_EXECUTABLE(client_main
  client/test_http_client_main.cc
)
TARGET_LINK_LIBRARIES(client_main
  liburlclient.a
  -lglog
  -lbase_shared
  -levent
  -lpthread
)
add_dependencies(client_main
  glog
  base_shared
)


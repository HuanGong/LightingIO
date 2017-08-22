set(LIBEVENT_PATH ${LightingIO_SOURCE_DIR}/thirdparty/libevent)
set(LIBEVENT_OUT_PATH ${CMAKE_BINARY_DIR}/thirdparty/libevent)

include_directories(${LightingIO_SOURCE_DIR})

include_directories(${LIBEVENT_PATH}/include)
include_directories(${LIBEVENT_OUT_PATH}/include)

SET(CMAKE_C_COMPILER g++)
ADD_DEFINITIONS(-std=c++11)

ADD_EXECUTABLE(lighting_srv
  test_http_server_main.cc
  http_server.cc
)

TARGET_LINK_LIBRARIES(lighting_srv
  -lglog
  -lbase_shared
  -levent
  -lpthread
)
add_dependencies(lighting_srv
  glog
  base_shared
)

# client side
ADD_LIBRARY(urlclient STATIC
  client/httpchannel_libevent.cc
)

TARGET_LINK_LIBRARIES(urlclient
  -lglog
  -lbase_shared
  -levent
  -lpthread
)
add_dependencies(urlclient
  glog
  base_shared
)

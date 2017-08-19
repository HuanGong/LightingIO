set(LIBEVENT_PATH ${LightingIO_SOURCE_DIR}/thirdparty/libevent)
set(LIBEVENT_OUT_PATH ${CMAKE_BINARY_DIR}/thirdparty/libevent)

include_directories(${LightingIO_SOURCE_DIR})

include_directories(${LIBEVENT_PATH}/include)
include_directories(${LIBEVENT_OUT_PATH}/include)

SET(CMAKE_C_COMPILER g++)
ADD_DEFINITIONS(-std=c++11)

#LINK_DIRECTORIES(${LIBEVENT_OUT_PATH}/lib)

ADD_EXECUTABLE(lighting_srv
  test_http_server_main.cc
  http_server.cc
)

TARGET_LINK_LIBRARIES(lighting_srv
  -lglog
  -levent
  -lgflags_nothreads
  -lbase_shared
  -lpthread
)
add_dependencies(lighting_srv
  base_shared
  gflags_nothreads
  gflags_nothreads_static
)



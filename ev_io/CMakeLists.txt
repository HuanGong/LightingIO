set(LIBEVENT_PATH ${LightingIO_SOURCE_DIR}/thirdparty/libevent)
set(LIBEVENT_OUT_PATH ${CMAKE_BINARY_DIR}/thirdparty/libevent)

include_directories(${LightingIO_SOURCE_DIR})

include_directories(${LIBEVENT_PATH}/include)
include_directories(${LIBEVENT_OUT_PATH}/include)

SET(CMAKE_C_COMPILER g++)
#add_compile_options(-std=c++11)
ADD_DEFINITIONS(-std=c++11)

set(IO_SOURCE
    ev_loop.cc
    http_server.cc)

ADD_LIBRARY(socktio ${IO_SOURCE})

# libevent.so path
LINK_DIRECTORIES(${LIBEVENT_OUT_PATH}/lib)

TARGET_LINK_LIBRARIES(socktio -levent)

# loop test
set(TEST_IO_SOURCE
    test_ev_loop_main.cc)

ADD_EXECUTABLE(ioloop_test ${TEST_IO_SOURCE})

TARGET_LINK_LIBRARIES(ioloop_test
  base_shared
  socktio
  libevent.a
  -lpthread)
#TARGET_LINK_LIBRARIES(ioloop_test base)


include_directories(${LightingIO_SOURCE_DIR})

include_directories(${LIBEVENT_PATH}/include)
include_directories(${LIBEVENT_OUT_PATH}/include)

SET(CMAKE_C_COMPILER g++)
ADD_DEFINITIONS(-std=c++11)

add_dependencies(ltnet
  glog
  base_shared
)
LINK_LIBRARIES(
	base_shared
)

ADD_EXECUTABLE(runloop_test
  test/run_loop_test.cc
)

ADD_EXECUTABLE(benchmark_test
  test/benchmark.cc
)


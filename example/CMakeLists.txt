
ADD_DEFINITIONS(-std=c++11)
include_directories(
  ${CMAKE_BINARY_DIR}/thirdparty
  ${CMAKE_BINARY_DIR}/thirdparty/libcoro

  ${CMAKE_BINARY_DIR}/thirdparty/glog
  ${CMAKE_SOURCE_DIR}/thirdparty/glog/src

  ${CMAKE_BINARY_DIR}/thirdparty/libevent/include
  ${CMAKE_SOURCE_DIR}/thirdparty/libevent/include

  ${CMAKE_BINARY_DIR}/thirdparty/gflags/include
  )

ADD_EXECUTABLE(coro_task_test
  coro_schedule_test.cc
  )

add_dependencies(coro_task_test
  base_shared
  coro
  glog
  )
TARGET_LINK_LIBRARIES(coro_task_test
  libcoro.a
  libglog.a
  libbase_static.a
  libgflags_nothreads.a
  -levent
  -lpthread
  )

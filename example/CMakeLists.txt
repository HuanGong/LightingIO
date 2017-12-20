ADD_DEFINITIONS(-std=c++11)

ADD_EXECUTABLE(coro_task_test
  coro_schedule_test.cc
)

add_dependencies(coro_task_test
  coro
  glog
  base_shared
)
TARGET_LINK_LIBRARIES(coro_task_test
  libcoro.a
  libglog.a
  -levent
  -lbase_shared
  -lpthread
)

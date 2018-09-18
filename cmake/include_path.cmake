# top level include dir

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/base

  #net
  ${CMAKE_SOURCE_DIR}/net

  ${CMAKE_SOURCE_DIR}/thirdparty
  ${CMAKE_BINARY_DIR}/thirdparty

  ${CMAKE_SOURCE_DIR}/components

  #coro
  ${CMAKE_BINARY_DIR}/thirdparty/libcoro

  ${GLOG_INCLUDE_DIR}
)

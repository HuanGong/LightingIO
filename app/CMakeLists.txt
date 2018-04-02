SET(CMAKE_C_COMPILER g++)
ADD_DEFINITIONS(-std=c++11)
ADD_DEFINITIONS(-fPIC)

set(LTCONTENT_SOURCE
  app_content.cc
  example_app.cc
)

SET(LTCONTENTEXE_LINKLIBS
  ltcontent
  ltnet
  ltbase
  ${GLOG_LIBRARY}
  ${ZLIB_LIBRARIES}
  http_parser
  coro
  gflags
  unwind
  pthread
)

ADD_LIBRARY(ltcontent STATIC
  ${LTCONTENT_SOURCE}
)
add_dependencies(ltcontent
  ltnet
  ltbase
)
add_executable(lt_app
  main.cc
)
TARGET_LINK_LIBRARIES(lt_app
  ${LTCONTENTEXE_LINKLIBS}
)


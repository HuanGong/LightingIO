SET(CMAKE_C_COMPILER g++)
ADD_DEFINITIONS(-std=c++11)
ADD_DEFINITIONS(-fPIC)

set(LTCONTENT_SOURCE
  app_content.cc
  example_app.cc
)

#ADD_LIBRARY(ltnet STATIC
ADD_LIBRARY(ltcontent SHARED
  ${LTCONTENT_SOURCE}
)

TARGET_LINK_LIBRARIES(ltcontent
  -lglog
  -lgflags
  -lltnet
  -lhttp_parser
  -lbase_shared
  -lpthread
)
add_dependencies(ltcontent
  ltnet
  base_shared
)

add_executable(lt_app
  main.cc
)
add_dependencies(lt_app
  ltcontent
)
TARGET_LINK_LIBRARIES(lt_app
  -lltcontent
  -lgflags
)


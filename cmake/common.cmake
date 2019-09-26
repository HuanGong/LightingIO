
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

find_package(ZLIB REQUIRED)
find_package(GLOG REQUIRED)
find_package(Gflags REQUIRED)
find_package(MYSQL REQUIRED)
find_package(Tcmalloc)

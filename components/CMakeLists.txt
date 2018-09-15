cmake_minimum_required(VERSION 2.8)

SET(CMAKE_C_COMPILER g++)

ADD_DEFINITIONS(-std=c++1y)
ADD_DEFINITIONS(-fPIC)

SET(LTCOMPONENT_DEPENDENCY
  3rd
)

SET(COMPONENT_SOURCES
  ./bloomfilter/bloom_filter.cc
  ./count_min_sketch/count_min_sketch.cc)

ADD_LIBRARY(ltcomponent STATIC
  ${COMPONENT_SOURCES}
)

add_dependencies(
  ltcomponent
  ${LTCOMPONENT_DEPENDENCY}
)

ADD_EXECUTABLE(bloomfilter_unittest
  ./bloomfilter/bloom_filter_test.cc
)
set_target_properties(bloomfilter_unittest PROPERTIES EXCLUDE_FROM_ALL TRUE)
TARGET_LINK_LIBRARIES(bloomfilter_unittest
  ltcomponent
  ${LTCOMPONENT_DEPENDENCY}
)
ADD_EXECUTABLE(countminsketch_unittest
  ./count_min_sketch/count_min_sketch_unittests.cc
)
set_target_properties(countminsketch_unittest PROPERTIES EXCLUDE_FROM_ALL TRUE)
TARGET_LINK_LIBRARIES(countminsketch_unittest
  ltcomponent
  ${LTCOMPONENT_DEPENDENCY}
)

ADD_EXECUTABLE(lrucache_unittest
  lru_cache/lru_cache_unittest.cpp
)
set_target_properties(lrucache_unittest PROPERTIES EXCLUDE_FROM_ALL TRUE)
TARGET_LINK_LIBRARIES(lrucache_unittest
  ltcomponent
  ${LTCOMPONENT_DEPENDENCY}
)

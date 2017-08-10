
SET(CMAKE_C_COMPILER g++)
#add_compile_options(-std=c++11)
ADD_DEFINITIONS(-std=c++11)

SET(BASE_SOURCES
    utils/ns_convertor.cc
    utils/ns_convertor.h)

ADD_LIBRARY(base SHARED ${BASE_SOURCES})

SET(BASE_UNITTESTS main.c)

ADD_EXECUTABLE(base_unittest ${BASE_UNITTESTS})
TARGET_LINK_LIBRARIES(base_unittest base)

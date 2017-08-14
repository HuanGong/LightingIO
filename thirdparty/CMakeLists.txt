#SET(CMAKE_C_COMPILER g++)
#ADD_DEFINITIONS(-std=c++11)
#add_compile_options(-std=c++11)

ADD_SUBDIRECTORY(glog)
ADD_SUBDIRECTORY(libco)
ADD_SUBDIRECTORY(libcoro)
ADD_SUBDIRECTORY(libevent)

INCLUDE_DIRECTORIES(
  glog/src
  )

SET(CRC_SOURCES
    crc/crc.cpp
    crc/crc.h)

ADD_LIBRARY(crc STATIC ${CRC_SOURCES})


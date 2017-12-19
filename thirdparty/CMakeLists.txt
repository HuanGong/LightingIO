
SET(gflags_DIR /usr/lib/x86_64-linux-gnu)
ADD_SUBDIRECTORY(glog)
ADD_SUBDIRECTORY(libcoro)
ADD_SUBDIRECTORY(libevent)
ADD_SUBDIRECTORY(rapidjson)

INCLUDE_DIRECTORIES(
  glog/src
)

SET(CRC_SOURCES
    crc/crc.cpp
    crc/crc.h)

ADD_LIBRARY(crc STATIC ${CRC_SOURCES})


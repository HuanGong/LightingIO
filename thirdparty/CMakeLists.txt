set(CMAKE_POSITION_INDEPENDENT_CODE ON)
SET(gflags_DIR /usr/lib/x86_64-linux-gnu)

ADD_SUBDIRECTORY(resp)
ADD_SUBDIRECTORY(libcoro)

SET(CRC_SOURCES
    crc/crc.cpp
)
SET(MD5_SOURCES
  md5/md5.c
)
SET(TINYXML_SOURCES
  tinyxml2/tinyxml2.cpp
)
SET(HTTPPARSER_SOURCES
  http_parser/http_parser.c
)
SET(URLSAFE64_SOURCES
  urlsafe64/urlsafe64.c
)

ADD_LIBRARY(crc STATIC ${CRC_SOURCES})

ADD_LIBRARY(http_parser STATIC ${HTTPPARSER_SOURCES})

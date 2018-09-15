set(CMAKE_POSITION_INDEPENDENT_CODE ON)
SET(gflags_DIR /usr/lib/x86_64-linux-gnu)

ADD_SUBDIRECTORY(resp)
ADD_SUBDIRECTORY(libcoro)

if (BUILD_CONCURRENTQUEUE_ALL)
  include(ExternalProject)
  ExternalProject_Add(concurrentqueue
    CONFIGURE_COMMAND ""
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/thirdparty/concurrentqueue
    UPDATE_COMMAND ""
    BUILD_COMMAND ${MAKE}
    # Specify build dir location
    BINARY_DIR ${PROJECT_SOURCE_DIR}/thirdparty/concurrentqueue/build
    #BUILD_IN_SOURCE 0
    INSTALL_COMMAND ""
  )
endif(BUILD_CONCURRENTQUEUE_ALL)

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
SET(MurMurHash3_SOURCES
  ./murmurhash/MurmurHash3.cpp
)

ADD_LIBRARY(3rd STATIC
  ${CRC_SOURCES}
  ${MD5_SOURCES}
  ${TINYXML_SOURCES}
  ${HTTPPARSER_SOURCES}
  ${URLSAFE64_SOURCES}
  ${MurMurHash3_SOURCES}
)

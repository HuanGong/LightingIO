

set(LIBEVENT_PATH ${LightingIO_SOURCE_DIR}/thirdparty/libevent)

include_directories(${LIBEVENT_PATH}/include)

SET(CMAKE_C_COMPILER g++)
#add_compile_options(-std=c++11)
ADD_COMPILE_OPTIONS(-std=c++11)

set(IO_SOURCE
    ev_loop.cc)

ADD_LIBRARY(socktio ${IO_SOURCE})
TARGET_LINK_LIBRARIES(socktio libevent)



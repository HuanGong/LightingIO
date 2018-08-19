#
# This file is part of the CMake build system for RESP
#
# Copyright (c) 2016 Nous Xiong (348944179 at qq dot com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# See https://github.com/nousxiong/resp for latest version.
#

file (GLOB_RECURSE RESP_EXAMPLE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable (resp_array ${RESP_EXAMPLE_FILES})

install (TARGETS resp_array RUNTIME DESTINATION bin)

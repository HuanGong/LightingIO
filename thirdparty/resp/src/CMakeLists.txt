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

# Not only x64 linux need this, but all linux gcc, we need this.
if (CMAKE_COMPILER_IS_GNUCXX AND NOT MINGW)
  add_definitions (-fPIC)
endif ()

# Set up all files.
file (GLOB_RECURSE RESP_HEADER_FILES "${PROJECT_SOURCE_DIR}/resp/*.hpp")
file (GLOB_RECURSE RESP_SOURCE_FILES "${PROJECT_SOURCE_DIR}/src/src.cpp")

# RESP library for dev.
add_library (resp STATIC ${RESP_SOURCE_FILES} ${RESP_HEADER_FILES})

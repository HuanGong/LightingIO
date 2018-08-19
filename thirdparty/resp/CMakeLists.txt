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

cmake_minimum_required (VERSION 2.8.6 FATAL_ERROR)
project (resp)

# Provide user options to customise the build process.
option (RESP_DEV "RESP dev mode" OFF)
option (RESP_BUILD_EXAMPLE "Build RESP examples" OFF)
option (RESP_BUILD_TEST "Build RESP tests" OFF)

include_directories (${PROJECT_SOURCE_DIR})

if (WIN32)
  # Set >= vc10 SAFESEH to NO.
  if (${CMAKE_GENERATOR} MATCHES "Visual Studio 1*")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
  endif ()

  # Set vc bigobj.
  if (${CMAKE_GENERATOR} MATCHES "Visual Studio *")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
  endif ()
endif ()

if (RESP_DEV)
  add_subdirectory (src)
endif ()

if (RESP_BUILD_EXAMPLE)
  add_subdirectory (example)
endif ()

if (RESP_BUILD_TEST)
  add_subdirectory (test)
endif ()

# All headers need install.
install (
  DIRECTORY ${PROJECT_SOURCE_DIR}/resp DESTINATION include
  PATTERN "CVS" EXCLUDE
  PATTERN ".svn" EXCLUDE
)

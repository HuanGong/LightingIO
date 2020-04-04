ADD_DEFINITIONS(
  -DUSE_TCMALLOC
  -DUSENONSTD_STRING_VIEW
  -DNET_ENABLE_REUSER_PORT
)
SET(USE_TCMALLOC TRUE)
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

# ---[ Set debug postfix
set(LtIO_DEBUG_POSTFIX "-d")
set(LtIO_POSTFIX "")
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/bin)

function(ltio_default_properties target)
  set_target_properties(${target} PROPERTIES
    DEBUG_POSTFIX ${LtIO_DEBUG_POSTFIX}
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTOR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTOR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUTIME_OUTPUT_DIRECTOR}"
    )
  # make sure we build all external dependencies first
  if (DEFINED external_project_dependencies)
    add_dependencies(${target} ${external_project_dependencies})
  endif()
endfunction()

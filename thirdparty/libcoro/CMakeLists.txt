cmake_minimum_required(VERSION 2.8)

include (CheckFunctionExists)
include (CheckIncludeFiles)
include (CheckSymbolExists)

project (libcoro)
add_library(coro coro.c coro.h)
set_target_properties(coro PROPERTIES COMPILE_DEFINITIONS "CORO_ASM=1")

check_include_files (ucontext.h HAVE_UCONTEXT_H)
check_include_files (setjmp.h HAVE_SETJMP_H)
check_symbol_exists (sigaltstack "signal.h" HAVE_SIGALTSTACK)

configure_file(config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
include_directories("${PROJECT_BINARY_DIR}")

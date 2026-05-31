cmake_minimum_required(VERSION 3.12)

add_library(upng INTERFACE)

target_sources(upng INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/upng.c

)
cmake_minimum_required(VERSION 3.5)

add_library(field INTERFACE)

target_sources(field INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/upng/upng.c
  ${CMAKE_CURRENT_LIST_DIR}/image_load.c
  ${CMAKE_CURRENT_LIST_DIR}/containers.c
  ${CMAKE_CURRENT_LIST_DIR}/primitives.c
  ${CMAKE_CURRENT_LIST_DIR}/field.c
  ${CMAKE_CURRENT_LIST_DIR}/fonts.c
  ${CMAKE_CURRENT_LIST_DIR}/curves.c

)

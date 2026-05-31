cmake_minimum_required(VERSION 3.5)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(FIELD)
find_package( OpenGL REQUIRED )

include_directories( ${OPENGL_INCLUDE_DIRS} )

set( GLFW_BUILD_DOCS OFF CACHE BOOL  "GLFW lib only" )
set( GLFW_INSTALL OFF CACHE BOOL  "GLFW lib only" )

# ADD_LIBRARY(field_glfw field_glfw.c)

add_subdirectory( glfw build/glfw)
add_subdirectory( field build/field )
add_subdirectory( cell build/cell )


set( FIELD
     main.c
     )

add_executable(loom
	main.c
)

target_link_libraries(loom ${OPENGL_LIBRARIES} glfw field cell)

target_compile_options(loom PRIVATE
    #-Wno-implicit-function-declaration
    -Wno-error=implicit-function-declaration
    # -O3
    #-Wno-implicit-int
    -Wno-error=implicit-int

    # 3. Incompatible pointer types (e.g., passing int* to char*)
    #-Wno-incompatible-pointer-types
    -Wno-error=incompatible-pointer-types

    -Wno-error=return-mismatch
)

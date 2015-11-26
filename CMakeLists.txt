cmake_minimum_required(VERSION 3.3)

project(fps_style_room)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_MODULE_PATH /usr/local/lib/cmake /usr/local/lib/x86_64-linux-gnu/cmake)
set(CMAKE_PREFIX_PATH /usr/local/lib/cmake/glfw )

#set(${GLM_DIR} /usr/local/lib/x86_64-linux-gnu/cmake/glm)

find_package (PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
find_package (OpenGL REQUIRED)
#find_package (glfw3 REQUIRED)
find_package (GLM REQUIRED)
find_package (GLEW REQUIRED STATIC)

set(SOURCE_FILES main.cpp utils/maths_funcs.cpp utils/maths_funcs.h utils/quat_funcs.cpp utils/quat_funcs.h)
#set(SOURCE_FILES main.cpp __add_other_cpp_files_here__)

#set additional libraries
set( ADDITIONAL_LIBS
        -pthread
        -lrt
        -lXinerama
        -lXrandr
        -lXxf86vm
        -lXi
        -lX11
        -lGLEW
        -lGLU
        -lglfw3
        -lm
        )

# Include directories for this project
set(INCLUDE_PATH
        ${OPENGL_INCLUDE_DIR}
        ${GLFW_INCLUDE_DIRS}
        )

# Libraries needed on all platforms for this project
set(LIBRARIES
        ${X11_LIBRARIES}
        ${ADDITIONAL_LIBS}
        ${GLEW_LIBRARY}
        ${GLFW_STATIC_LIBRARIES}
        ${OPENGL_LIBRARIES}
        )

add_executable(fps_style_room ${SOURCE_FILES})


include_directories(${CMAKE_SOURCE_DIR} ${INCLUDE_PATH})
target_link_libraries (fps_style_room ${LIBRARIES})
cmake_minimum_required(VERSION 3.6)
project(solar_system)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
add_definitions(${OpenGL_DEFINITIONS})

find_package(GLUT REQUIRED)
include_directories(${GLUT_INCLUDE_DIRS})
link_directories(${GLUT_LIBRARY_DIRS})
add_definitions(${GLUT_DEFINITIONS})

set(SOURCE_FILES main.c tga.c)
add_executable(solar_system ${SOURCE_FILES})
target_link_libraries(solar_system m ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})

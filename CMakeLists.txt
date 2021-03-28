cmake_minimum_required(VERSION 3.14)
set(PROJECT_NAME MMCG)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 11)

find_package(OpenGL REQUIRED)

add_subdirectory(external)

set(SOURCE_FILES
    src/glad.c
    src/main.cpp
    src/ShaderProgram.cpp)

set(ADDITIONAL_INCLUDE_DIRS
    external/glad/include)
include_directories(${ADDITIONAL_INCLUDE_DIRS})

add_executable(main ${SOURCE_FILES})

target_include_directories(main
    PRIVATE
    ${OPENGL_INCLUDE_DIR}
    include)
target_link_libraries(main
    LINK_PUBLIC
    ${OPENGL_gl_LIBRARY}
    glfw)
target_compile_options(main PRIVATE -Werror -Wall -Wextra)

add_custom_command(TARGET main POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/shaders" "${PROJECT_BINARY_DIR}")

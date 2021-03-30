cmake_minimum_required(VERSION 3.14)
set(PROJECT_NAME MMCG)
project(${PROJECT_NAME})

include(ExternalProject)

set(CMAKE_CXX_STANDARD 14)

#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------
# dependencies
#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------
find_package(OpenGL REQUIRED)

#add external libraries which are compatable with CMake
add_subdirectory(external)

#include external library headers with SYSTEM flag
#(otherwise compilation may fail because of -Werror -Wall -Wextra)
set(HEADER_ONLY_INCLUDE_DIRS
    external/glad/include
    external/stb)
include_directories(SYSTEM ${HEADER_ONLY_INCLUDE_DIRS})

# download releases of header-only libraries

#download nlohmann/json
ExternalProject_Add(
    json
    URL https://github.com/nlohmann/json/archive/refs/tags/v3.9.1.zip
    INSTALL_COMMAND ""
    BUILD_COMMAND ""
    CONFIGURE_COMMAND ""
)
ExternalProject_Get_Property(json SOURCE_DIR)
set(JSON_INCLUDE_DIRS "${SOURCE_DIR}/include")

#downlaod eigen
ExternalProject_Add(
    eigen
    URL https://gitlab.com/libeigen/eigen/-/archive/3.3.9/eigen-3.3.9.zip
    INSTALL_COMMAND ""
    BUILD_COMMAND ""
    CONFIGURE_COMMAND ""
)
ExternalProject_Get_Property(eigen SOURCE_DIR)
set(EIGEN3_INCLUDE_DIRS ${SOURCE_DIR})

#downlaod eigen
ExternalProject_Add(
    glm
    URL https://github.com/g-truc/glm/releases/download/0.9.9.8/glm-0.9.9.8.zip
    INSTALL_COMMAND ""
    BUILD_COMMAND ""
    CONFIGURE_COMMAND ""
)
ExternalProject_Get_Property(glm SOURCE_DIR)
set(GLM_INCLUDE_DIRS ${SOURCE_DIR})

#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------
# executable
#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------

set(SOURCE_FILES
    src/glad.c
    src/main.cpp
    src/ShaderProgram.cpp
    src/glError.cpp
    src/Texture.cpp
    src/Camera.cpp)

add_executable(main ${SOURCE_FILES})

add_dependencies(main eigen json)

target_include_directories(main
    PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIRS}
    ${JSON_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    include)

target_link_libraries(main
    PRIVATE
    ${OPENGL_gl_LIBRARY}
    glfw)

target_compile_options(main PRIVATE -Werror -Wall -Wextra -Wno-unused-parameter)

#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------
# copy shaders
#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------

add_custom_command(TARGET main POST_BUILD COMMAND ${CMAKE_COMMAND} -E
    copy_directory "${PROJECT_SOURCE_DIR}/shaders" "${PROJECT_BINARY_DIR}")

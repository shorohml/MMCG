cmake_minimum_required(VERSION 3.14)
set(PROJECT_NAME MMCG)
project(${PROJECT_NAME})

include(FetchContent)

set(CMAKE_CXX_STANDARD 11)

#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------
# dependencies
#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------
find_package(OpenGL REQUIRED)

# add external libraries which are compatable with CMake
add_subdirectory(external)

#include external library headers with SYSTEM flag
#(otherwise compilation may fail because of -Werror -Wall -Wextra)
set(HEADER_ONLY_INCLUDE_DIRS
    external/glad/include
    external/stb)
include_directories(SYSTEM ${HEADER_ONLY_INCLUDE_DIRS})

# featch light version of nlohmann/json
FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
  GIT_TAG v3.9.1)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  FetchContent_Populate(json)
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------
# executable
#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------

set(SOURCE_FILES
    src/glad.c
    src/main.cpp
    src/ShaderProgram.cpp
    src/glError.cpp)

add_executable(main ${SOURCE_FILES})

target_include_directories(main
    PRIVATE
    ${OPENGL_INCLUDE_DIR}
    include)

target_link_libraries(main
    PRIVATE
    ${OPENGL_gl_LIBRARY}
    glfw
    nlohmann_json::nlohmann_json)

target_compile_options(main PRIVATE -Werror -Wall -Wextra)

#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------
# copy shaders and config
#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------

add_custom_command(TARGET main POST_BUILD COMMAND ${CMAKE_COMMAND} -E
    copy_directory "${PROJECT_SOURCE_DIR}/shaders" "${PROJECT_BINARY_DIR}")

cmake_minimum_required(VERSION 3.14)
set(PROJECT_NAME MMCG)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 14)

#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------
# dependencies
#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------

find_package(OpenGL REQUIRED)

add_subdirectory(external)

#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------
# executable
#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------

set(CMAKE_BUILD_TYPE Release)

set(SOURCE_FILES
    src/glad.c
    src/main.cpp
    src/ShaderProgram.cpp
    src/GLError.cpp
    src/Camera.cpp
    src/App.cpp
    src/Models/Mesh.cpp
    src/Models/Texture.cpp
    src/Models/ImportScene.cpp)

add_executable(main ${SOURCE_FILES})

include_directories(SYSTEM
    ${STB_INCLUDE_DIRS})
target_include_directories(main
    PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${GLAD_INCLUDE_DIRS}
    include)

target_link_libraries(main
    PRIVATE
    ${OPENGL_gl_LIBRARY}
    glfw
    assimp
    glm
    nlohmann_json)

target_compile_options(main PRIVATE -Werror -Wall -Wextra)

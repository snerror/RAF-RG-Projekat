cmake_minimum_required(VERSION 3.17)
project(RAF_RG_Projekat_VladetaPutnikovic)

include(ExternalProject)
find_package(OpenGL REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(lib/glm)

# glfw
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
add_subdirectory(lib/glfw)

include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
        ${PROJECT_SOURCE_DIR}/lib/assimp/lib
)

set(
        PROJECT_SOURCES
        src/app.cpp
        src/glad.c
        src/shader.h
        src/stb_image.h
        src/stb_image.cpp
        src/camera.h
        src/mesh.h
        src/model.h
        src/map_generator.h)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_link_libraries(
        ${PROJECT_NAME}
        ${OPENGL_LIBRARIES}
        glfw
        glm
)

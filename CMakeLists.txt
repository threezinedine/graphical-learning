cmake_minimum_required(VERSION 3.20)

set(CMAKE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_DIR})

macro(ntt_find_package name)
    set(${name}_DIR ${CMAKE_DIR})
    find_package(${name} REQUIRED)
endmacro()


project(GraphicalApp LANGUAGES CXX)

ntt_find_package(ntt-glfw)
ntt_find_package(ntt-stb)
ntt_find_package(ntt-profiler)
ntt_find_package(ntt-glm)
ntt_find_package(ntt-assimp)

## OpenGL Application
file(
    GLOB_RECURSE 
    OPENGL_SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src-opengl/*.cpp"
)

file(
    GLOB_RECURSE 
    OPENGL_HEADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src-opengl/*.h"
)

set(OPENGL_PROJECT_NAME ${PROJECT_NAME}-opengl)

add_executable(
    ${OPENGL_PROJECT_NAME} 
    ${OPENGL_SOURCE_FILES} 
    ${OPENGL_HEADER_FILES}
)


target_link_libraries(
    ${OPENGL_PROJECT_NAME}
    PRIVATE
    ntt-glfw
    ntt-stb
    ntt-profiler
    ntt-glm
    ntt-assimp
)

target_compile_definitions(
    ${OPENGL_PROJECT_NAME}
    PRIVATE
    SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(
    ${OPENGL_PROJECT_NAME}
    PRIVATE
    -g
)

## Vulkan Application
file(
    GLOB_RECURSE 
    VULKAN_SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src-vulkan/*.cpp"
)

file(
    GLOB_RECURSE 
    VULKAN_HEADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src-vulkan/*.h"
)

set(VULKAN_PROJECT_NAME ${PROJECT_NAME}-vulkan)

add_executable(
    ${VULKAN_PROJECT_NAME}
    ${VULKAN_SOURCE_FILES}
    ${VULKAN_HEADER_FILES}
)

target_link_libraries(
    ${VULKAN_PROJECT_NAME}
    PRIVATE
    ntt-glfw
    ntt-stb
    ntt-profiler
    ntt-glm
    ntt-assimp
)

target_compile_definitions(
    ${VULKAN_PROJECT_NAME}
    PRIVATE
    SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(
    ${VULKAN_PROJECT_NAME}
    PRIVATE
    -g
)
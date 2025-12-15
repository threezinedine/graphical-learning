cmake_minimum_required(VERSION 3.20)

set(CMAKE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_DIR})

macro(ntt_find_package name)
    set(${name}_DIR ${CMAKE_DIR})
    find_package(${name} REQUIRED)
endmacro()


project(GraphicalApp LANGUAGES CXX)

file(
    GLOB_RECURSE 
    SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

file(
    GLOB_RECURSE 
    HEADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_executable(
    ${PROJECT_NAME} 
    ${SOURCE_FILES} 
    ${HEADER_FILES}
)

ntt_find_package(ntt-glfw)
ntt_find_package(ntt-stb)
ntt_find_package(ntt-profiler)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    ntt-glfw
    ntt-stb
    ntt-profiler
)

target_compile_definitions(
    ${PROJECT_NAME}
    PRIVATE
    SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(
    ${PROJECT_NAME}
    PRIVATE
    -g
)
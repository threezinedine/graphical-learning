set(SET_LIBRARY_NAME ntt-glfw)

if (TARGET ${SET_LIBRARY_NAME})
    return()
endif()

if (NOT TARGET glfw)
    include(FetchContent)

    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        latest
    )

    FetchContent_MakeAvailable(glfw)
endif()

if (NOT TARGET GLAD)
    include(FetchContent)

    FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/threezinedine/glad.git
    )
    FetchContent_MakeAvailable(glad)
endif()

if (NOT TARGET OpenGL::GL)
    find_package(OpenGL REQUIRED)
endif()

add_library(
    ${SET_LIBRARY_NAME} 
    INTERFACE
)

target_link_libraries(
    ${SET_LIBRARY_NAME} 
    INTERFACE
    glfw
    glad
    OpenGL::GL
)
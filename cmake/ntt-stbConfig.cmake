set(LIB_NAME ntt-stb)

if (TARGET ${LIB_NAME})
    return()
endif()

if (NOT TARGET stb)
    include(FetchContent)

    FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/threezinedine/stb.git
    )

    FetchContent_MakeAvailable(stb)
endif()

add_library(
    ${LIB_NAME} 
    INTERFACE
)

target_link_libraries(
    ${LIB_NAME} 
    INTERFACE
    stb
)
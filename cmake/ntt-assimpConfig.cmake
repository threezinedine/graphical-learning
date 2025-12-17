set(LIB_NAME ntt-assimp)

if (TARGET ${LIB_NAME} )
    return()
endif()

if (NOT TARGET assimp)
    include(FetchContent)

    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
    )

    FetchContent_MakeAvailable(assimp)
endif()

add_library(${LIB_NAME} INTERFACE)
target_link_libraries(${LIB_NAME} INTERFACE assimp::assimp)
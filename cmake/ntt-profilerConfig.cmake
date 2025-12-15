set(LIB_NAME ntt-profiler)

if (TARGET ${LIB_NAME})
    return()
endif()


if (NOT TARGET easy_profiler)
    include(FetchContent)

    FetchContent_Declare(
        easy_profiler
        GIT_REPOSITORY https://github.com/yse/easy_profiler.git
        GIT_TAG        develop
    )
    set(EASY_PROFILER_NO_GUI ON)
    FetchContent_MakeAvailable(easy_profiler)
endif()

add_library(
    ${LIB_NAME} 
    INTERFACE
)

target_link_libraries(
    ${LIB_NAME} 
    INTERFACE
    easy_profiler
)
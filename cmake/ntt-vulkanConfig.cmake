set(LIB_NAME ntt-vulkan)

if (TARGET ${LIB_NAME})
    return()
endif()

if (NOT TARGET Vulkan)
    find_package(Vulkan REQUIRED)
endif()

add_library(${LIB_NAME} INTERFACE)
target_link_libraries(${LIB_NAME} INTERFACE Vulkan::Vulkan)

find_program(Vulkan::glslc Vulkan::glslc)
set(GLSLC_EXECUTABLE Vulkan::glslc)

macro(ntt_vulkan_compile shaderFile outputDir)
    get_filename_component(FILENAME ${shaderFile} NAME) 
    get_filename_component(FILENAME_WE ${shaderFile} NAME_WE)

    add_custom_target(
        ${FILENAME_WE} ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${outputDir}
        COMMAND ${GLSLC_EXECUTABLE} -o ${outputDir}/${FILENAME}.spv ${shaderFile}
        DEPENDS ${shaderFile}
        COMMENT "Compiling Vulkan shader: ${FILENAME} to ${FILENAME}.spv"
    )
endmacro()

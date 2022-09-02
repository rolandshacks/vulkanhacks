################################################################################
#
# Vulkan build support
#
################################################################################

set(VULKAN_SDK $ENV{VULKAN_SDK})
set(VULKAN_INCLUDE ${VULKAN_SDK}/include)
set(VULKAN_LIB ${VULKAN_SDK}/lib)
set(VULKAN_BIN ${VULKAN_SDK}/bin)

add_definitions(-DSDL_MAIN_HANDLED)
include_directories(SYSTEM ${VULKAN_INCLUDE})
link_directories(${VULKAN_LIB})

find_package(Vulkan COMPONENTS glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

function(compile_shader target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "ENV;FORMAT" "SOURCES")
    foreach(source ${arg_SOURCES})
        add_custom_command(
            OUTPUT ${source}.${arg_FORMAT}
            DEPENDS ${source}
            DEPFILE ${source}.d
            COMMAND
                ${glslc_executable}
                $<$<BOOL:${arg_ENV}>:--target-env=${arg_ENV}>
                $<$<BOOL:${arg_FORMAT}>:-mfmt=${arg_FORMAT}>
                -MD -MF ${source}.d
                -o ${source}.${arg_FORMAT}
                ${CMAKE_CURRENT_SOURCE_DIR}/${source}
        )
        target_sources(${target} PRIVATE ${source}.${arg_FORMAT})
    endforeach()
endfunction()

################################################################################
# Example
################################################################################
#
# compile_shader(${PROJECT_NAME}
#     ENV vulkan1.2
#     FORMAT num
#     SOURCES
#         shader.vert
#         shader.frag
# )
#
################################################################################

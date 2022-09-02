################################################################################
#
# Gamekit Compiler
#
################################################################################

cmake_minimum_required(VERSION 3.20)
cmake_policy(VERSION 3.20)

find_package (Python COMPONENTS Interpreter)
set(GAMEKITC_PYTHON_EXECUTABLE ${Python_EXECUTABLE} CACHE INTERNAL "")
set(GAMEKITC_EXECUTABLE ${CMAKE_SOURCE_DIR}/gamekit/tools/gamekitc.py CACHE INTERNAL "")

function(compile_resources TARGET)
    add_custom_command(
        OUTPUT ${TARGET}.cpp
        DEPFILE ${TARGET}.d
        COMMAND
            ${GAMEKITC_PYTHON_EXECUTABLE} ${GAMEKITC_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}
            ${TARGET}
    )
    target_sources(${TARGET} PRIVATE ${TARGET}.cpp)
endfunction()

function(export_folder FOLDERNAME)
    file(GLOB_RECURSE FILES LIST_DIRECTORIES false RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/${FOLDERNAME}" "${CMAKE_CURRENT_SOURCE_DIR}/${FOLDERNAME}/**")
    add_custom_target(exporting ALL)
    add_custom_command(TARGET exporting POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "exporting files"
    )
    foreach(FILE ${FILES})
        add_custom_command(TARGET exporting POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${FOLDERNAME}/${FILE} ${CMAKE_CURRENT_BINARY_DIR}/${FOLDERNAME}/${FILE}
        )
    endforeach(FILE)
endfunction()

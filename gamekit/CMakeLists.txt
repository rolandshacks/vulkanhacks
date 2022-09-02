################################################################################
#
# Gamekit Library
#
################################################################################

cmake_minimum_required(VERSION 3.20)
cmake_policy(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(gamekit)

set(INCLUDE_DIR include/gamekit)
set(SOURCE_DIR src)

set(INCLUDE_FILES
    ${INCLUDE_DIR}/gamekit.h
    ${INCLUDE_DIR}/api.h
    ${INCLUDE_DIR}/loader.h
    ${INCLUDE_DIR}/primitives.h
    ${INCLUDE_DIR}/types.h
    ${INCLUDE_DIR}/resources.h
    ${INCLUDE_DIR}/utilities.h
    ${INCLUDE_DIR}/device.h
    ${INCLUDE_DIR}/reference.h
    ${INCLUDE_DIR}/types.h
    ${INCLUDE_DIR}/metrics.h
    ${INCLUDE_DIR}/material.h
    ${INCLUDE_DIR}/vertex.h
    ${INCLUDE_DIR}/frame.h
    ${INCLUDE_DIR}/buffer.h
    ${INCLUDE_DIR}/texture.h
    ${INCLUDE_DIR}/clock.h
    ${INCLUDE_DIR}/application.h
    ${INCLUDE_DIR}/window.h
    ${INCLUDE_DIR}/sprite.h
    ${INCLUDE_DIR}/sprite_batch.h
)

set(SOURCE_FILES
    ${SOURCE_DIR}/api.cpp
    ${SOURCE_DIR}/loader.cpp
    ${SOURCE_DIR}/utilities.cpp
    ${SOURCE_DIR}/device.cpp
    ${SOURCE_DIR}/types.cpp
    ${SOURCE_DIR}/metrics.cpp
    ${SOURCE_DIR}/resources.cpp
    ${SOURCE_DIR}/material.cpp
    ${SOURCE_DIR}/vertex.cpp
    ${SOURCE_DIR}/frame.cpp
    ${SOURCE_DIR}/buffer.cpp
    ${SOURCE_DIR}/texture.cpp
    ${SOURCE_DIR}/clock.cpp
    ${SOURCE_DIR}/application.cpp
    ${SOURCE_DIR}/window.cpp
    ${SOURCE_DIR}/sprite.cpp
    ${SOURCE_DIR}/sprite_batch.cpp
)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES} ${INCLUDE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} SDL2 volk)

set(GAMEKIT_SDK ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(GAMEKIT_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)

include(tools/gamekitc.cmake)

################################################################################
#
# Particles Demo
#
################################################################################

cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(particles)

add_subdirectory("resources")
#export_folder("assets")

set(INCLUDE_FILES
    src/entity.h
)

set(SOURCE_FILES
    src/entity.cpp
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${INCLUDE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${GAMEKIT_INCLUDE})
target_link_libraries(${PROJECT_NAME} gamekit ${PROJECT_NAME}_resources)

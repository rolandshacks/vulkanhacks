################################################################################
#
# Pre-compiled assets library
#
################################################################################

cmake_minimum_required(VERSION 3.20)
cmake_policy(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(${PROJECT_NAME}_resources)

add_library(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE ${GAMEKIT_INCLUDE} ${CMAKE_CURRENT_BINARY_DIR})
compile_resources(${PROJECT_NAME})

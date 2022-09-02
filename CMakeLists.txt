################################################################################
#
# Main cmake file
#
################################################################################

cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(vulkan_works)

# Math lib
include(glm.cmake)

# Image lib
include(stb.cmake)

# Vulkan bindings
include(volk.cmake)

# Vulkan SDK
include(vulkan.cmake)

# JSON lib
include(json.cmake)

# Gamekit library
add_subdirectory(gamekit)

# Hello world demo
add_subdirectory(hello)

# Particles demo
add_subdirectory(particles)

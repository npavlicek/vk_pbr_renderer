cmake_minimum_required(VERSION 3.26)
project(vk_pbr_renderer)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_PREFIX_PATH "lib/cmake")

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(vk_pbr_renderer main.cpp
        ansi_color_defs.h
        Util.cpp
        Util.h)

target_link_libraries(vk_pbr_renderer Vulkan::Vulkan)
target_link_libraries(vk_pbr_renderer glfw)

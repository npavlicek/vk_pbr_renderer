cmake_minimum_required(VERSION 3.26)
project(vk_pbr_renderer)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_PREFIX_PATH "lib/cmake")

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(vk_pbr_renderer main.cpp
        App.cpp
        App.h
        vk_init_utils.cpp
        vk_init_utils.h
        ansi_color_defs.h)

target_link_libraries(vk_pbr_renderer Vulkan::Vulkan)
target_link_libraries(vk_pbr_renderer glfw)

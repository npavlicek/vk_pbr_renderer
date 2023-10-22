cmake_minimum_required(VERSION 3.26)
project(vk_pbr_renderer)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_PREFIX_PATH "lib/cmake")

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

include_directories(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/imgui
        ${PROJECT_SOURCE_DIR}/imgui/backends
)

file(GLOB proj_srcs
        "${PROJECT_SOURCE_DIR}/*.cpp"
        "${PROJECT_SOURCE_DIR}/imgui/*.cpp"
        "${PROJECT_SOURCE_DIR}/imgui/backends/*.cpp")

add_executable(vk_pbr_renderer ${proj_srcs})

target_link_libraries(vk_pbr_renderer Vulkan::Vulkan)
target_link_libraries(vk_pbr_renderer glfw)

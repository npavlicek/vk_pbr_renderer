cmake_minimum_required(VERSION 3.25)
project(vk_pbr_renderer)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_PREFIX_PATH "lib/cmake")

option(ENABLE_VALIDATION_LAYERS "Enable Vulkan validation layers")

if (${ENABLE_VALIDATION_LAYERS})
    add_compile_definitions(ENABLE_VULKAN_VALIDATION_LAYERS)
endif ()

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

include_directories(
		${PROJECT_SOURCE_DIR}
		${PROJECT_SOURCE_DIR}/imgui
		${PROJECT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/src/include
)

file(GLOB proj_srcs
		"${PROJECT_SOURCE_DIR}/src/*.cpp"
		"${PROJECT_SOURCE_DIR}/imgui/*.cpp"
		"${PROJECT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp"
		"${PROJECT_SOURCE_DIR}/imgui/backends/imgui_impl_vulkan.cpp"
)

add_executable(vk_pbr_renderer ${proj_srcs})

target_link_libraries(vk_pbr_renderer Vulkan::Vulkan)
target_link_libraries(vk_pbr_renderer glfw)

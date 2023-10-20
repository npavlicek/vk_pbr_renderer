#ifndef VK_PBR_RENDERER_UTIL_H
#define VK_PBR_RENDERER_UTIL_H

#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>
#include <GLFW/glfw3.h>

#include <iostream>

namespace util {
	vk::raii::Instance createInstance(vk::raii::Context &context, const char *applicationName, const char *engineName);
	vk::raii::PhysicalDevice selectPhysicalDevice(vk::raii::Instance &instance);
	int selectQueueFamily(vk::raii::PhysicalDevice &physicalDevice);
	vk::raii::Device createDevice(vk::raii::PhysicalDevice &physicalDevice);
	vk::raii::SurfaceKHR createSurface(vk::raii::Instance &instance, GLFWwindow *window);
	vk::raii::SwapchainKHR
	createSwapChain(vk::raii::Device &device, vk::raii::PhysicalDevice &physicalDevice, vk::raii::SurfaceKHR &surface);
} // util

#endif //VK_PBR_RENDERER_UTIL_H

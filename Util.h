#ifndef VK_PBR_RENDERER_UTIL_H
#define VK_PBR_RENDERER_UTIL_H

#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>

namespace util {
	vk::raii::Instance createInstance(
			vk::raii::Context &context,
			const char *applicationName,
			const char *engineName
	);
	vk::raii::PhysicalDevice selectPhysicalDevice(
			vk::raii::Instance &instance
	);
	int selectQueueFamily(
			vk::raii::PhysicalDevice &physicalDevice
	);
	vk::raii::Device createDevice(
			vk::raii::PhysicalDevice &physicalDevice,
			int queueFamilyIndex
	);
	vk::raii::SurfaceKHR createSurface(
			vk::raii::Instance &instance,
			GLFWwindow *window
	);
	vk::SurfaceFormatKHR selectSwapChainFormat(
			vk::raii::PhysicalDevice &physicalDevice,
			vk::raii::SurfaceKHR &surface
	);
	vk::raii::SwapchainKHR createSwapChain(
			vk::raii::Device &device,
			vk::raii::PhysicalDevice &physicalDevice,
			vk::raii::SurfaceKHR &surface,
			vk::SurfaceFormatKHR format,
			vk::SurfaceCapabilitiesKHR capabilities
	);
	vk::raii::CommandPool createCommandPool(
			vk::raii::Device &device,
			int queueFamilyIndex
	);
	vk::raii::CommandBuffers createCommandBuffers(
			vk::raii::Device &device,
			vk::raii::CommandPool &commandPool,
			int count
	);
	std::vector<vk::raii::ImageView> createImageViews(
			vk::raii::Device &device,
			std::vector<vk::Image> &images,
			vk::SurfaceFormatKHR &swapChainFormat
	);
	std::vector<vk::raii::ShaderModule> createShaderModules(
			vk::raii::Device &device,
			const char *vertexShaderPath,
			const char *fragmentShaderPath
	);
	vk::raii::Pipeline createPipeline(
			vk::raii::Device &device,
			vk::raii::RenderPass &renderPass,
			std::vector<vk::raii::ShaderModule> &shaderModules,
			vk::SurfaceCapabilitiesKHR surfaceCapabilities,
			vk::SurfaceFormatKHR surfaceFormat
	);
	vk::raii::RenderPass createRenderPass(
			vk::raii::Device &device,
			vk::SurfaceFormatKHR surfaceFormat
	);
	std::vector<vk::raii::Framebuffer> createFrameBuffers(
			vk::raii::Device &device,
			vk::raii::RenderPass &renderPass,
			std::vector<vk::raii::ImageView> &imageViews,
			vk::SurfaceCapabilitiesKHR surfaceCapabilities
	);
} // util

#endif //VK_PBR_RENDERER_UTIL_H

#pragma once

#include <iostream>
#include <fstream>
#include <optional>
#include <exception>

#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>
#include <GLFW/glfw3.h>

#include "Vertex.h"
#include "Validation.h"
#include "VkErrorHandling.h"

namespace util
{
	vk::raii::Instance createInstance(
		vk::raii::Context &context,
		const char *applicationName,
		const char *engineName,
		std::optional<vk::DebugUtilsMessengerCreateInfoEXT> debugUtilsMessengerCreateInfo = {});
	vk::raii::PhysicalDevice selectPhysicalDevice(
		vk::raii::Instance &instance);
	int selectQueueFamily(
		vk::raii::PhysicalDevice &physicalDevice);
	vk::raii::Device createDevice(
		vk::raii::PhysicalDevice &physicalDevice,
		int queueFamilyIndex);
	vk::raii::SurfaceKHR createSurface(
		vk::raii::Instance &instance,
		GLFWwindow *window);
	vk::SurfaceFormatKHR selectSwapChainFormat(
		vk::raii::PhysicalDevice &physicalDevice,
		vk::raii::SurfaceKHR &surface);
	std::pair<vk::raii::SwapchainKHR, vk::SwapchainCreateInfoKHR> createSwapChain(
		vk::raii::Device &device,
		vk::raii::PhysicalDevice &physicalDevice,
		vk::raii::SurfaceKHR &surface,
		vk::SurfaceFormatKHR format,
		vk::SurfaceCapabilitiesKHR capabilities);
	vk::raii::CommandPool createCommandPool(
		vk::raii::Device &device,
		int queueFamilyIndex);
	vk::raii::CommandBuffers createCommandBuffers(
		vk::raii::Device &device,
		vk::raii::CommandPool &commandPool,
		int count);
	vk::Format selectDepthFormat(vk::raii::PhysicalDevice &physicalDevice);
	std::vector<vk::raii::ImageView> createImageViews(
		vk::raii::Device &device,
		std::vector<vk::Image> &images,
		vk::Format format,
		vk::ImageAspectFlags imageAspectFlags);
	vk::raii::ImageView createImageView(
		vk::raii::Device &device,
		vk::Image image,
		vk::Format format,
		vk::ImageAspectFlags imageAspectFlags);
	std::vector<vk::raii::ShaderModule> createShaderModules(
		vk::raii::Device &device,
		const char *vertexShaderPath,
		const char *fragmentShaderPath);
	std::tuple<vk::raii::Pipeline, vk::raii::PipelineLayout, vk::raii::PipelineCache, vk::raii::DescriptorSetLayout>
	createPipeline(
		vk::raii::Device &device,
		vk::raii::RenderPass &renderPass,
		std::vector<vk::raii::ShaderModule> &shaderModules);
	vk::raii::RenderPass createRenderPass(
		vk::raii::Device &device,
		vk::SurfaceFormatKHR surfaceFormat,
		vk::Format depthFormat);
	std::vector<vk::raii::Framebuffer> createFrameBuffers(
		vk::raii::Device &device,
		vk::raii::RenderPass &renderPass,
		std::vector<vk::raii::ImageView> &imageViews,
		std::vector<vk::raii::ImageView> &depthImageViews,
		vk::SurfaceCapabilitiesKHR surfaceCapabilities);
	vk::raii::DescriptorPool createDescriptorPool(vk::raii::Device &device);
	void uploadVertexData(
		vk::raii::Device &device,
		vk::raii::DeviceMemory &stagingBufferMemory,
		const std::vector<Vertex> &vertices);
	std::tuple<vk::raii::Image, vk::raii::DeviceMemory> createImage(
		vk::raii::Device &device,
		vk::raii::PhysicalDevice &physicalDevice,
		vk::ImageTiling tiling,
		vk::ImageUsageFlags imageUsageFlags,
		vk::Format format,
		vk::Extent3D extent,
		vk::ImageType imageType,
		vk::MemoryPropertyFlags memoryPropertyFlags);
	void uploadIndexData(
		vk::raii::Device &device,
		vk::raii::DeviceMemory &stagingBufferMemory,
		const std::vector<uint16_t> &indices);
	std::tuple<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(
		vk::raii::Device &device,
		vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties,
		vk::Flags<vk::MemoryPropertyFlagBits> memoryProperties,
		vk::DeviceSize bufferSize,
		vk::Flags<vk::BufferUsageFlagBits> usage);
	std::vector<vk::raii::DescriptorSet> createDescriptorSets(
		vk::raii::Device &device,
		vk::raii::DescriptorPool &descriptorPool,
		vk::raii::DescriptorSetLayout &descriptorSetLayout,
		int descriptorSetCount);
	uint32_t findMemoryIndex(
		vk::MemoryRequirements memoryRequirements,
		vk::MemoryPropertyFlags memoryPropertyFlags,
		vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties);
} // util

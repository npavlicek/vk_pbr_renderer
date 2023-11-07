#pragma once

#include <iostream>
#include <fstream>
#include <optional>
#include <exception>

#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>
#include <vma/vk_mem_alloc.h>
#include <GLFW/glfw3.h>

#include "Mesh.h"
#include "Validation.h"
#include "VkErrorHandling.h"
#include "Renderer.h"

namespace util
{
	struct Image
	{
		vk::Image handle;
		VmaAllocation allocation;
		vk::ImageCreateInfo createInfo;
	};

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
	std::tuple<vk::raii::Image, vk::raii::DeviceMemory> createImage(
		vk::raii::Device &device,
		vk::raii::PhysicalDevice &physicalDevice,
		vk::ImageTiling tiling,
		vk::ImageUsageFlags imageUsageFlags,
		vk::Format format,
		vk::Extent3D extent,
		vk::ImageType imageType,
		vk::MemoryPropertyFlags memoryPropertyFlags);
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
	/**
	 * @brief Create an image. Prefer calling this function before any other allocations as it requests VMA to create dedicated memory.
	 * Also allocates only on the GPU, can be edited to request CPU memory from VMA if needed.
	 *
	 * @param allocator vma allocator handle
	 * @param format
	 * @param extent
	 * @param mipLevels
	 * @param sampleCount
	 * @param imageUsage
	 * @return Image struct containing allocation info and vulkan image handle
	 */
	Image createImage2(
		const VmaAllocator &allocator,
		vk::Format format,
		vk::Extent3D extent,
		int mipLevels,
		vk::SampleCountFlagBits sampleCount,
		vk::ImageUsageFlags imageUsage);
	/**
	 * @brief create an image view
	 *
	 * @param device
	 * @param image
	 * @param format
	 * @param imageAspectFlags
	 * @return vk::ImageView
	 */
	vk::ImageView createImageView2(
		const vk::Device &device,
		const vk::Image &image,
		vk::Format format,
		vk::ImageAspectFlags imageAspectFlags);
} // util

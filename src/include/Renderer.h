#ifndef RENDERER_H_
#define RENDERER_H_

#include "Util.h"
#include "CommandBuffer.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

class Renderer
{
public:
	Renderer(GLFWwindow *window);
	Renderer(const Renderer &rhs) = delete;
	Renderer(const Renderer &&rhs) = delete;

	void uploadVertexData(std::vector<Vertex> vertices);

private:
	vk::raii::Context context;
	vk::raii::Instance instance;
	vk::raii::DebugUtilsMessengerEXT debugMessenger;
	vk::raii::PhysicalDevice physicalDevice;
	vk::raii::Device device;
	vk::raii::SurfaceKHR surface;
	vk::raii::SwapchainKHR swapChain;
	vk::raii::CommandPool commandPool;
	vk::raii::Pipeline pipeline;
	vk::raii::PipelineLayout pipelineLayout;
	vk::raii::PipelineCache pipelineCache;
	vk::raii::DescriptorSetLayout descriptorSetLayout;
	vk::raii::RenderPass renderPass;
	vk::raii::Queue queue;

	std::vector<vk::Image> swapChainImages;
	std::vector<vk::raii::ImageView> swapChainImageViews;
	std::vector<vk::raii::CommandBuffer> commandBuffers;
	std::vector<vk::raii::ShaderModule> shaderModules;
	std::vector<vk::raii::Framebuffer> frameBuffers;
	std::vector<vk::raii::Image> depthImages;
	std::vector<vk::raii::DeviceMemory> depthImageMemorys;
	std::vector<vk::raii::ImageView> depthImageViews;

	vk::SurfaceFormatKHR swapChainFormat;
	vk::SwapchainCreateInfoKHR swapChainCreateInfo;
	vk::SurfaceCapabilitiesKHR swapChainSurfaceCapabilities;
	vk::Format depthImageFormat;

	VmaAllocator vmaAllocator;

	int queueFamilyGraphicsIndex;

	void createSwapChain();
	void createDepthBuffers();
};

#endif // RENDERER_H_

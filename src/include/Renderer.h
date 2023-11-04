#pragma once

#include <chrono>

#include "Util.h"
#include "CommandBuffer.h"
#include "Texture.h"
#include "VkErrorHandling.h"
#include "Frame.h"
#include "Model.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/common.hpp>
// #include <glm/gtx/string_cast.hpp>

#include <vma/vk_mem_alloc.h>

struct UniformData
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

// FIXME: im not sure why this is required, i need to figure out how to fix the error i get without it.
class Texture;

class Renderer
{
public:
	Renderer() = delete;
	Renderer(GLFWwindow *window);
	Renderer(const Renderer &rhs) = delete;
	Renderer(const Renderer &&rhs) = delete;
	~Renderer();

	void uploadUniformData(const UniformData &uniformData, int frame);
	void updateDescriptorSets(const Texture &tex);
	Texture createTexture(const char *path);
	Model createModel(const char *path);
	void loop(const std::vector<Model> &models);
	void destroy();
	void destroyModel(Model &model);

private:
	// TODO: Convert back to non raii vk handles
	vk::raii::Context context;
	vk::raii::Instance instance{nullptr};
	vk::raii::DebugUtilsMessengerEXT debugMessenger{nullptr};
	vk::raii::PhysicalDevice physicalDevice{nullptr};
	vk::raii::Device device{nullptr};
	vk::raii::SurfaceKHR surface{nullptr};
	vk::raii::SwapchainKHR swapChain{nullptr};
	vk::raii::CommandPool commandPool{nullptr};
	vk::raii::Pipeline pipeline{nullptr};
	vk::raii::PipelineLayout pipelineLayout{nullptr};
	vk::raii::PipelineCache pipelineCache{nullptr};
	vk::raii::DescriptorSetLayout descriptorSetLayout{nullptr};
	vk::raii::RenderPass renderPass{nullptr};
	vk::raii::Queue queue{nullptr};
	vk::raii::DescriptorPool descriptorPool{nullptr};

	std::vector<vk::Image> swapChainImages;
	std::vector<vk::DescriptorSet> descriptorSets;
	std::vector<vk::raii::ImageView> swapChainImageViews;
	std::vector<vk::raii::CommandBuffer> commandBuffers;
	std::vector<vk::raii::ShaderModule> shaderModules;
	std::vector<vk::raii::Framebuffer> frameBuffers;
	std::vector<vk::raii::Image> depthImages;
	std::vector<vk::raii::DeviceMemory> depthImageMemorys;
	std::vector<vk::raii::ImageView> depthImageViews;
	std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
	std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
	std::vector<vk::raii::Fence> inFlightFences;

	vk::SurfaceFormatKHR swapChainFormat;
	vk::SwapchainCreateInfoKHR swapChainCreateInfo;
	vk::SurfaceCapabilitiesKHR swapChainSurfaceCapabilities;
	vk::Format depthImageFormat;

	VmaAllocator vmaAllocator;

	VmaAllocation vertexBufferAllocation;
	vk::Buffer vertexBuffer;

	VmaAllocation indexBufferAllocation;
	vk::Buffer indexBuffer;

	// TODO: delete the unused uniform buffers, using push constants instead
	std::vector<VmaAllocation> uniformBufferAllocations;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<void *> uniformBufferPtr;

	GLFWwindow *window;
	ImGuiContext *imGuiContext;

	VkResCheck res;

	UniformData ubo;

	// TODO: temp
	int numIndices;

	vk::ClearColorValue clearColorValue;
	vk::ClearDepthStencilValue clearDepthValue;
	std::vector<vk::ClearValue> clearValues;
	vk::Rect2D renderArea;
	// TODO: end temp

	int queueFamilyGraphicsIndex;
	int framesInFlight = 2;

	void createDescriptorObjects();
	void createSyncObjects();
	void createSwapChain();
	void createDepthBuffers();
	void createUniformBuffers();
	void initializeImGui();
};

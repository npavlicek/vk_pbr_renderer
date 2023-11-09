#pragma once

#include <chrono>

#include "CommandBuffer.h"
#include "Frame.h"
#include "Model.h"
#include "Texture.h"
#include "Util.h"
#include "VkErrorHandling.h"

#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>

#include <glm/common.hpp>
#include <glm/ext.hpp>
#include <glm/glm.hpp>

// #include <glm/gtx/string_cast.hpp>

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_structs.hpp>

// FIXME: temporary
struct ModelSettings
{
	struct
	{
		float x, y, z;
	} pos;
	struct
	{
		float x, y, z;
	} rotation;
};

struct UniformData
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

struct RenderInfo
{
	glm::vec3 cameraPos;
	glm::vec3 lightPos[4];
};

class Renderer
{
  public:
	Renderer() = delete;
	Renderer(GLFWwindow *window);
	Renderer(const Renderer &rhs) = delete;
	Renderer(const Renderer &&rhs) = delete;
	~Renderer();

	Texture createTexture(const char *path);
	Model createModel(const char *path);
	void render(const std::vector<Model> &models, glm::vec3 cameraPos, glm::mat4 view);
	void destroy();
	void destroyModel(Model &model);
	void resetCommandBuffers();

  private:
	// TODO: Convert back to non raii vk handles
	vk::Instance instance;
	vk::DebugUtilsMessengerEXT debugMessenger;
	vk::PhysicalDevice physicalDevice;
	vk::Device device;

	int graphicsQueueIndex;
	vk::Queue graphicsQueue;

	std::vector<vk::CommandPool> commandPools;

	vk::raii::SurfaceKHR surface{nullptr};
	vk::raii::SwapchainKHR swapChain{nullptr};
	vk::raii::Pipeline pipeline{nullptr};
	vk::raii::PipelineLayout pipelineLayout{nullptr};
	vk::raii::PipelineCache pipelineCache{nullptr};
	vk::raii::DescriptorSetLayout descriptorSetLayout{nullptr};
	vk::raii::RenderPass renderPass{nullptr};
	vk::raii::Queue queue{nullptr};
	vk::raii::DescriptorPool descriptorPool{nullptr};
	vk::raii::DescriptorSetLayout textureSetLayout;
	vk::raii::DescriptorSetLayout renderInfoLayout;

	std::vector<vk::Image> swapChainImages;
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
	vk::SampleCountFlagBits msaaSamples;
	vk::Sampler sampler;
	std::vector<vk::raii::DescriptorSet> descriptorSet;

	util::Image multisampledImage;
	vk::ImageView multisampledImageView;

	VmaAllocator vmaAllocator;

	VmaAllocation vertexBufferAllocation;
	vk::Buffer vertexBuffer;

	VmaAllocation indexBufferAllocation;
	vk::Buffer indexBuffer;

	VkBuffer renderInfoBuffer;
	VmaAllocation renderInfoBufferAlloc;
	VmaAllocationInfo renderInfoBufferAllocInfo;

	GLFWwindow *window;
	ImGuiContext *imGuiContext;

	VkResCheck res;

	UniformData ubo;
	RenderInfo renderInfo;

	// TODO: temp
	int numIndices;

	vk::ClearColorValue clearColorValue;
	vk::ClearDepthStencilValue clearDepthValue;
	std::vector<vk::ClearValue> clearValues;
	vk::Rect2D renderArea;

	ModelSettings modelSettings{{0.f, 5.f, 2.f}, {}};
	// TODO: end temp

	int queueFamilyGraphicsIndex;
	int framesInFlight = 2;
	int currentFrame = 0;

	void createInstance();
	void selectPhysicalDevice();
	void selectGraphicsQueue();
	void createDevice();
	void createCommandPools();

	void createDescriptorSetLayouts();
	void createDescriptorSets();
	void updateDescriptorSets();
	void createUniformBuffers();
	void writeRenderInfo();
	void createSyncObjects();
	void createSwapChain();
	void createDepthBuffers();
	void createMultisampledImageTarget();
	void detectSampleCounts();
	void initializeImGui();
};

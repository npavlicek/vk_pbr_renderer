#pragma once

#include <chrono>

#include "CommandBuffer.h"
#include "Frame.h"
#include "Model.h"
#include "PBRPipeline.h"
#include "RenderPass.h"
#include "SwapChain.h"
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
#include <vulkan/vulkan_enums.hpp>
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

struct ImageObject
{
	vk::Image image;
	VmaAllocation imageAllocation;
	VmaAllocationInfo imageAllocationInfo;
	vk::ImageView imageView;
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
	vk::Instance instance;
	vk::DebugUtilsMessengerEXT debugMessenger;
	vk::PhysicalDevice physicalDevice;
	vk::Device device;
	vk::SurfaceKHR surface;
	N::SwapChain swapChain;
	N::RenderPass renderPass;
	vk::Queue graphicsQueue;
	N::PBRPipeline pipeline;

	vk::Sampler sampler;

	int graphicsQueueIndex;
	vk::SampleCountFlagBits samples;
	vk::Format depthFormat;

	std::vector<vk::CommandPool> commandPools;
	std::vector<ImageObject> depthImages;
	std::vector<ImageObject> renderTargets;
	std::vector<vk::Framebuffer> frameBuffers;

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
	void createSurface();
	void detectSampleCounts();
	void selectDepthFormat();
	void createDepthObjects();
	void createRenderTargets();
	void createFrameBuffers();

	void createDescriptorSets();
	void updateDescriptorSets();
	void createUniformBuffers();
	void writeRenderInfo();
	void createSyncObjects();
	void initializeImGui();
};

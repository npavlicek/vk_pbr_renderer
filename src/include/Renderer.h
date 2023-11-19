#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>
#include <vk_mem_alloc.h>
#include <GLFW/glfw3.h>

#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>

#include "Model.h"
#include "PBRPipeline.h"
#include "RenderPass.h"
#include "SwapChain.h"

namespace N
{
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

// TODO: temporary
struct CameraSettings
{
	glm::vec3 pos;
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

	Model createModel(const char *path);
	void render(std::vector<Model> &models, glm::vec3 cameraPos, glm::mat4 view);
	void destroy();
	void destroyModel(Model &model);

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
	vk::DescriptorPool descriptorPool;
	vk::CommandPool commandPool;

	int graphicsQueueIndex;
	vk::SampleCountFlagBits samples;
	vk::Format depthFormat;

	std::vector<ImageObject> depthImages;
	std::vector<ImageObject> renderTargets;
	std::vector<vk::Framebuffer> frameBuffers;
	std::vector<vk::CommandBuffer> commandBuffers;
	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences;

	int framesInFlight = 2;
	int currentFrame = 0;

	VmaAllocator vmaAllocator;

	GLFWwindow *window;
	ImGuiContext *imGuiContext;

	vk::ClearColorValue clearColorValue;
	vk::ClearDepthStencilValue clearDepthValue;
	std::vector<vk::ClearValue> clearValues;

	ModelSettings modelSettings{{0.f, 5.f, 2.f}, {}};
	N::MVPPushConstant mvpPushConstant;

	void createInstance();
	void selectPhysicalDevice();
	void selectGraphicsQueue();
	void createDevice();
	void createCommandPool();
	void createSurface();
	void detectSampleCounts();
	void selectDepthFormat();
	void createDepthObjects();
	void createRenderTargets();
	void createFrameBuffers();
	void createDescriptorPool();
	void initializeImGui();
	void createCommandBuffers();
	void createSyncObjects();

	// TODO: temp
	void createDescriptorSet();
	vk::DescriptorSet cameraSettingsSet;

	vk::Buffer cameraSettingsBuffer;
	VmaAllocation cameraSettingsBufferAllocation;
	VmaAllocationInfo cameraSettingsAllocInfo;
};
} // namespace N

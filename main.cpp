// KEEP VULKAN INCLUDED AT THE TOP OR THINGS WILL BREAK
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_to_string.hpp>

#include "Vertex.h"
#include "Util.h"
#include "Frame.h"
#include "VkErrorHandling.h"

const int MAX_FRAMES_IN_FLIGHT = 2;
int currentFrame = 0;

const std::vector<Vertex> vertices{
		{{-0.5f, -0.5f}, {0.f, 1.f, 1.f}},
		{{0.5f,  -0.5f}, {0.f, 1.f, 0.f}},
		{{0.5f,  0.5f},  {0.f, 1.f, 0.f}},
		{{-0.5f, 0.5f},  {0.f, 0.f, 1.f}}
};

const std::vector<uint16_t> indices{
		0, 1, 2, 2, 3, 0
};

void keyCallback(
		GLFWwindow *window,
		int key,
		int scancode,
		int action,
		int mods
) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(
				window,
				GLFW_TRUE
		);
	}
}

int main() {
	if (!glfwInit())
		throw std::runtime_error("Could not initialize GLFW");

	glfwWindowHint(
			GLFW_RESIZABLE,
			GLFW_FALSE
	);
	glfwWindowHint(
			GLFW_CLIENT_API,
			GLFW_NO_API
	);

	GLFWwindow *window = glfwCreateWindow(
			1280,
			720,
			"Testing Vulkan!",
			nullptr,
			nullptr
	);

	if (!window) {
		std::cerr << "Could not open primary window!" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(
			window,
			keyCallback
	);

	VkResCheck res;

	vk::raii::Context context;
	auto instance = util::createInstance(
			context,
			"Vulkan PBR Renderer",
			"Vulkan PBR Renderer"
	);
	auto physicalDevice = util::selectPhysicalDevice(instance);
	auto queueFamilyIndex = util::selectQueueFamily(physicalDevice);
	auto device = util::createDevice(
			physicalDevice,
			queueFamilyIndex
	);
	auto surface = util::createSurface(
			instance,
			window
	);
	auto swapChainFormat = util::selectSwapChainFormat(
			physicalDevice,
			surface
	);
	auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);

	vk::raii::SwapchainKHR swapChain{nullptr};
	vk::SwapchainCreateInfoKHR swapChainCreateInfo;
	std::tie(
			swapChain,
			swapChainCreateInfo
	) = util::createSwapChain(
			device,
			physicalDevice,
			surface,
			swapChainFormat,
			surfaceCapabilities
	);

	auto images = swapChain.getImages();
	auto imageViews = util::createImageViews(
			device,
			images,
			swapChainFormat
	);
	auto commandPool = util::createCommandPool(
			device,
			queueFamilyIndex
	);
	auto commandBuffers = util::createCommandBuffers(
			device,
			commandPool,
			MAX_FRAMES_IN_FLIGHT
	);
	auto shaderModules = util::createShaderModules(
			device,
			"shaders/vert.spv",
			"shaders/frag.spv"
	);
	auto renderPass = util::createRenderPass(
			device,
			swapChainFormat
	);
	vk::raii::Pipeline pipeline{nullptr};
	vk::raii::PipelineCache pipelineCache{nullptr};
	std::tie(
			pipeline,
			pipelineCache
	) = util::createPipeline(
			device,
			renderPass,
			shaderModules,
			surfaceCapabilities,
			swapChainFormat
	);
	auto frameBuffers = util::createFrameBuffers(
			device,
			renderPass,
			imageViews,
			surfaceCapabilities
	);
	auto queue = device.getQueue(
			queueFamilyIndex,
			0
	);

	// Create staging and vertex buffers and upload data
	auto physicalDeviceMemoryProperties = physicalDevice.getMemoryProperties();
	const auto vertexBufferSize = sizeof(vertices[0]) * vertices.size();

	vk::raii::Buffer stagingBuffer{nullptr};
	vk::raii::DeviceMemory stagingBufferMemory{nullptr};
	std::tie(
			stagingBuffer,
			stagingBufferMemory
	) = util::createBuffer(
			device,
			physicalDeviceMemoryProperties,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			vertexBufferSize,
			vk::BufferUsageFlagBits::eTransferSrc
	);

	vk::raii::Buffer vertexBuffer{nullptr};
	vk::raii::DeviceMemory vertexBufferMemory{nullptr};
	std::tie(
			vertexBuffer,
			vertexBufferMemory
	) = util::createBuffer(
			device,
			physicalDeviceMemoryProperties,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vertexBufferSize,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer
	);

	util::uploadVertexData(
			device,
			stagingBufferMemory,
			vertices
	);
	//

	// Create index buffers and staging buffers and upload index data
	const auto indexBufferSize = sizeof(indices[0]) * indices.size();

	vk::raii::Buffer indexBufferStaging{nullptr};
	vk::raii::DeviceMemory indexBufferStagingMemory{nullptr};
	std::tie(
			indexBufferStaging,
			indexBufferStagingMemory
	) = util::createBuffer(
			device,
			physicalDeviceMemoryProperties,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			indexBufferSize,
			vk::BufferUsageFlagBits::eTransferSrc
	);

	vk::raii::Buffer indexBuffer{nullptr};
	vk::raii::DeviceMemory indexBufferMemory{nullptr};
	std::tie(
			indexBuffer,
			indexBufferMemory
	) = util::createBuffer(
			device,
			physicalDeviceMemoryProperties,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			indexBufferSize,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer
	);

	util::uploadIndexData(
			device,
			indexBufferStagingMemory,
			indices
	);
	//

	// Copy staging buffer data to vertex buffer
	{
		vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.setCommandBufferCount(1);
		commandBufferAllocateInfo.setCommandPool(*commandPool);
		commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);
		auto commandBuffer = std::move(vk::raii::CommandBuffers{device, commandBufferAllocateInfo}[0]);

		vk::CommandBufferBeginInfo commandBufferBeginInfo;
		commandBufferBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer.begin(commandBufferBeginInfo);

		vk::BufferCopy bufferCopy;
		bufferCopy.setSize(vertexBufferSize);
		commandBuffer.copyBuffer(
				*stagingBuffer,
				*vertexBuffer,
				bufferCopy
		);

		commandBuffer.end();

		vk::SubmitInfo submitInfo;
		submitInfo.setCommandBufferCount(1);
		submitInfo.setCommandBuffers(*commandBuffer);

		queue.submit(submitInfo);

		device.waitIdle();
	}
	//

	// Copy staging buffer data to index buffer
	{
		vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.setCommandBufferCount(1);
		commandBufferAllocateInfo.setCommandPool(*commandPool);
		commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);
		auto commandBuffer = std::move(vk::raii::CommandBuffers{device, commandBufferAllocateInfo}[0]);

		vk::CommandBufferBeginInfo commandBufferBeginInfo;
		commandBufferBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer.begin(commandBufferBeginInfo);

		vk::BufferCopy bufferCopy;
		bufferCopy.setSize(indexBufferSize);
		commandBuffer.copyBuffer(
				*indexBufferStaging,
				*indexBuffer,
				bufferCopy
		);

		commandBuffer.end();

		vk::SubmitInfo submitInfo;
		submitInfo.setCommandBufferCount(1);
		submitInfo.setCommandBuffers(*commandBuffer);

		queue.submit(submitInfo);

		device.waitIdle();
	}
	//

	std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
	std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
	imageAvailableSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		imageAvailableSemaphores.push_back(device.createSemaphore({}));
		renderFinishedSemaphores.push_back(device.createSemaphore({}));
	}

	std::vector<vk::raii::Fence> inFlightFences;
	inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		inFlightFences.push_back(device.createFence({vk::FenceCreateFlagBits::eSignaled}));
	}

	vk::ClearColorValue clearColorValue{0.f, 0.f, 0.f, 1.f};
	vk::Rect2D renderArea{
			{0, 0},
			surfaceCapabilities.currentExtent
	};

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		res = device.waitForFences(
				*inFlightFences[currentFrame],
				VK_TRUE,
				UINT32_MAX
		);
		device.resetFences(*inFlightFences[currentFrame]);

		uint32_t imageIndex;
		std::tie(
				res,
				imageIndex
		) = swapChain.acquireNextImage(
				UINT64_MAX,
				*imageAvailableSemaphores[currentFrame],
				VK_NULL_HANDLE
		);

		pbr::Frame::begin(
				commandBuffers[currentFrame],
				pipeline,
				vertexBuffer,
				indexBuffer
		);
		pbr::Frame::beginRenderPass(
				commandBuffers[currentFrame],
				renderPass,
				frameBuffers[imageIndex],
				clearColorValue,
				renderArea
		);
		pbr::Frame::draw(
				commandBuffers[currentFrame],
				renderArea,
				static_cast<int>(indices.size())
		);
		pbr::Frame::endRenderPass(commandBuffers[currentFrame]);
		pbr::Frame::end(commandBuffers[currentFrame]);

		std::vector<vk::PipelineStageFlags> pipelineStageFlags{vk::PipelineStageFlagBits::eColorAttachmentOutput};

		vk::SubmitInfo submitInfo;
		submitInfo.setWaitSemaphoreCount(1);
		submitInfo.setWaitSemaphores(*imageAvailableSemaphores[currentFrame]);
		submitInfo.setWaitDstStageMask(pipelineStageFlags);
		submitInfo.setCommandBufferCount(1);
		submitInfo.setCommandBuffers(*commandBuffers[currentFrame]);
		submitInfo.setSignalSemaphoreCount(1);
		submitInfo.setSignalSemaphores(*renderFinishedSemaphores[currentFrame]);

		queue.submit(
				submitInfo,
				*inFlightFences[currentFrame]
		);

		vk::PresentInfoKHR presentInfo;
		presentInfo.setSwapchainCount(1);
		presentInfo.setSwapchains(*swapChain);
		presentInfo.setImageIndices(imageIndex);
		presentInfo.setWaitSemaphoreCount(1);
		presentInfo.setWaitSemaphores(*renderFinishedSemaphores[currentFrame]);

		res = queue.presentKHR(presentInfo);

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
	device.waitIdle();

	glfwTerminate();
	return 0;
}

// KEEP VULKAN INCLUDED AT THE TOP OR THINGS WILL BREAK
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_to_string.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <glm/ext.hpp>

#include <chrono>

#include "Vertex.h"
#include "Util.h"
#include "Frame.h"
#include "VkErrorHandling.h"

const int MAX_FRAMES_IN_FLIGHT = 2;
int currentFrame = 0;

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

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
	vk::raii::PipelineLayout pipelineLayout{nullptr};
	vk::raii::PipelineCache pipelineCache{nullptr};
	vk::raii::DescriptorSetLayout descriptorSetLayout{nullptr};
	std::tie(
			pipeline,
			pipelineLayout,
			pipelineCache,
			descriptorSetLayout
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

	// UNIFORM BUFFERS

	void *uniformBufferMemPtrs[MAX_FRAMES_IN_FLIGHT];
	std::vector<vk::raii::Buffer> uniformBuffers;
	std::vector<vk::raii::DeviceMemory> uniformBufferMemory;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		auto uniformBuffer = util::createBuffer(
				device,
				physicalDeviceMemoryProperties,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
				sizeof(UniformBufferObject),
				vk::BufferUsageFlagBits::eUniformBuffer
		);
		uniformBuffers.push_back(std::move(std::get<vk::raii::Buffer>(uniformBuffer)));
		uniformBufferMemory.push_back(std::move(std::get<vk::raii::DeviceMemory>(uniformBuffer)));

		vkMapMemory(
				*device,
				*uniformBufferMemory[i],
				0,
				sizeof(UniformBufferObject),
				0,
				&uniformBufferMemPtrs[i]
		);
	}

	UniformBufferObject ubo{};
	ubo.projection = glm::perspective(
			glm::radians(45.f),
			1280.f / 720.f,
			0.1f,
			10.f
	);
	ubo.projection[1][1] *= -1;
	ubo.view = glm::lookAt(
			glm::vec3{2.f, 2.f, 2.f},
			glm::vec3{0.f, 0.f, 0.f},
			glm::vec3{0.f, 0.f, 1.f}
	);
	ubo.model = glm::identity<glm::mat4>();

	for (const auto &uniformBufferMemPtr: uniformBufferMemPtrs) {
		memcpy(
				uniformBufferMemPtr,
				&ubo,
				sizeof(ubo));
	}

	// END UNIFORM BUFFERS

	// DESCRIPTOR SETS

	auto descriptorPool = util::createDescriptorPool(
			device,
			MAX_FRAMES_IN_FLIGHT
	);

	auto descriptorSet = util::createDescriptorSet(
			device,
			descriptorPool,
			descriptorSetLayout
	);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::DescriptorBufferInfo descriptorBufferInfo{};
		descriptorBufferInfo.setOffset(0);
		descriptorBufferInfo.setBuffer(*uniformBuffers[i]);
		descriptorBufferInfo.setRange(sizeof(UniformBufferObject));

		vk::WriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.setDstSet(*descriptorSet);
		writeDescriptorSet.setDstBinding(0);
		writeDescriptorSet.setDstArrayElement(0);
		writeDescriptorSet.setDescriptorType(vk::DescriptorType::eUniformBuffer);
		writeDescriptorSet.setDescriptorCount(1);
		writeDescriptorSet.setBufferInfo(descriptorBufferInfo);

		device.updateDescriptorSets(
				writeDescriptorSet,
				nullptr
		);
	}

	// END DESCRIPTOR SETS


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

	auto startTime = std::chrono::high_resolution_clock::now();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		res = device.waitForFences(
				*inFlightFences[currentFrame],
				VK_TRUE,
				UINT32_MAX
		);
		device.resetFences(*inFlightFences[currentFrame]);

		// Update model matrix

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		//std::cout << "Time: " << time << "\n";
		std::cout << "Angle: " << time * glm::radians(180.f) << "\n";
		ubo.model = glm::rotate(
				glm::mat4(1.f),
				time * glm::radians(90.f),
				glm::vec3(
						0.f,
						0.f,
						1.f
				));

		memcpy(
				uniformBufferMemPtrs[currentFrame],
				&ubo,
				sizeof(ubo)
		);

		//

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
				pipelineLayout,
				vertexBuffer,
				indexBuffer,
				descriptorSet
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

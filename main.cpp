// KEEP VULKAN INCLUDED AT THE TOP OR THINGS WILL BREAK
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_to_string.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <glm/ext.hpp>

#define TINYOBJLOADER_IMPLEMENTATION

#include "tiny_obj_loader.h"

#include <chrono>
#include <string>

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

std::tuple<std::vector<Vertex>, std::vector<uint32_t>> loadObj(const char *filename) {
	tinyobj::ObjReaderConfig config{};
	config.triangulate = false;
	tinyobj::ObjReader objReader{};
	objReader.ParseFromFile(
			filename,
			config
	);

	const tinyobj::attrib_t &attrib = objReader.GetAttrib();
	const std::vector<tinyobj::shape_t> &shapes = objReader.GetShapes();
	//const std::vector<tinyobj::material_t> &materials = objReader.GetMaterials();

	std::srand(std::time(nullptr));

	std::vector<Vertex> vertices{};

	// MAKE SURE TO USE THE RIGHT INTEGER TYPE
	// Using 16 bits restricts us to small indices
	std::vector<uint32_t> indices{};

	for (int vertexIndex = 0; vertexIndex < static_cast<int>(attrib.vertices.size()); vertexIndex += 3) {
		Vertex vertex{};
		vertex.pos[0] = attrib.vertices[vertexIndex];
		vertex.pos[1] = attrib.vertices[vertexIndex + 1];
		vertex.pos[2] = attrib.vertices[vertexIndex + 2];
		//float color = (static_cast<float>(std::rand()) / RAND_MAX) * 0.5f;
		vertex.color[0] = static_cast<float>(std::rand()) / RAND_MAX;
		vertex.color[1] = static_cast<float>(std::rand()) / RAND_MAX;
		vertex.color[2] = static_cast<float>(std::rand()) / RAND_MAX;
		vertices.push_back(vertex);
	}

	for (const auto &shape: shapes) {
		for (const auto &index: shape.mesh.indices) {
			indices.push_back(index.vertex_index);
		}
	}

	return {
			vertices,
			indices
	};
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
			1920,
			1080,
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
			swapChainFormat.format,
			vk::ImageAspectFlagBits::eColor
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

	// BEGIN DEPTH IMAGE

	vk::Format depthImageFormat = util::selectDepthFormat(physicalDevice);

	vk::raii::DeviceMemory depthImageMemory{nullptr};
	vk::raii::Image depthImage{nullptr};
	std::tie(
			depthImage,
			depthImageMemory
	) = util::createImage(
			device,
			physicalDevice,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			depthImageFormat,
			{
					surfaceCapabilities.currentExtent.width,
					surfaceCapabilities.currentExtent.height,
					1
			},
			vk::ImageType::e2D
	);

	std::vector<vk::Image> depthImages{*depthImage};

	vk::raii::ImageView depthImageView = std::move(
			util::createImageViews(
					device,
					depthImages,
					depthImageFormat,
					vk::ImageAspectFlagBits::eDepth
			)[0]
	);

	depthImages.clear();

	// END DEPTH IMAGE

	auto renderPass = util::createRenderPass(
			device,
			swapChainFormat,
			depthImageFormat
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
			shaderModules
	);
	auto queue = device.getQueue(
			queueFamilyIndex,
			0
	);

	auto frameBuffers = util::createFrameBuffers(
			device,
			renderPass,
			imageViews,
			depthImageView,
			surfaceCapabilities
	);

	std::vector<Vertex> vertices{};
	std::vector<uint32_t> indices{};
	std::tie(
			vertices,
			indices
	) = loadObj("models/bunny.obj");

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
		auto commandBuffer = std::move(
				vk::raii::CommandBuffers{
						device,
						commandBufferAllocateInfo
				}[0]
		);

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
		auto commandBuffer = std::move(
				vk::raii::CommandBuffers{
						device,
						commandBufferAllocateInfo
				}[0]
		);

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
			1920.f / 1080.f,
			0.1f,
			2000.f
	);
	ubo.view = glm::lookAt(
			glm::vec3{
					5.f,
					5.f,
					5.f
			},
			glm::vec3{
					0.f,
					0.f,
					0.f
			},
			glm::vec3{
					0.f,
					0.f,
					1.f
			}
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
			device
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

	// BEGIN IMGUI

	auto imguiContext = ImGui::CreateContext();
	ImGui_ImplGlfw_InitForVulkan(
			window,
			true
	);

	ImGui_ImplVulkan_InitInfo imGuiImplVulkanInitInfo{
			*instance,
			*physicalDevice,
			*device,
			static_cast<uint32_t>(queueFamilyIndex),
			*queue,
			*pipelineCache,
			*descriptorPool,
			0,
			surfaceCapabilities.minImageCount,
			surfaceCapabilities.minImageCount,
			static_cast<VkSampleCountFlagBits>(vk::SampleCountFlagBits::e1),
			false,
			static_cast<VkFormat>(swapChainFormat.format),
			nullptr,
			nullptr
	};

	ImGui_ImplVulkan_Init(
			&imGuiImplVulkanInitInfo,
			*renderPass
	);

	// UPLOAD IMGUI FONTS
	{
		vk::raii::CommandBuffer commandBuffer = std::move(
				util::createCommandBuffers(
						device,
						commandPool,
						1
				)[0]
		);

		vk::CommandBufferBeginInfo commandBufferBeginInfo;
		commandBufferBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer.begin(commandBufferBeginInfo);

		ImGui_ImplVulkan_CreateFontsTexture(*commandBuffer);

		commandBuffer.end();
		vk::SubmitInfo submitInfo;
		submitInfo.setCommandBufferCount(1);
		submitInfo.setCommandBuffers(*commandBuffer);
		queue.submit(submitInfo);

		device.waitIdle();
	}

	ImGui_ImplVulkan_DestroyFontUploadObjects();

	// END IMGUI

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

	vk::ClearColorValue clearColorValue{
			0.f,
			0.f,
			0.f,
			1.f
	};
	vk::ClearDepthStencilValue clearDepthValue{
			1.f,
			0
	};
	std::vector<vk::ClearValue> clearValues{
			clearColorValue,
			clearDepthValue
	};
	vk::Rect2D renderArea{
			{
					0,
					0
			},
			surfaceCapabilities.currentExtent
	};

	auto startTime = std::chrono::high_resolution_clock::now();

	float input = 0.f;
	bool x = false, y = false, z = true;
	struct Pos {
		float x;
		float y;
		float z;
	} pos;
	pos.x = 5.f;
	pos.y = 5.f;
	pos.z = 5.f;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		res = device.waitForFences(
				*inFlightFences[currentFrame],
				VK_TRUE,
				UINT32_MAX
		);
		device.resetFences(*inFlightFences[currentFrame]);

		// IMGUI NEW FRAME

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("Settings");
		ImGui::Text("Rotation");
		ImGui::SliderFloat(
				"Degrees/S",
				&input,
				-180.f,
				180.f
		);
		ImGui::Checkbox(
				"X",
				&x
		);
		ImGui::Checkbox(
				"Y",
				&y
		);
		ImGui::Checkbox(
				"Z",
				&z
		);
		ImGui::Text("Camera Pos");
		ImGui::SliderFloat(
				"X Pos",
				&pos.x,
				-1000.f,
				1000.f
		);
		ImGui::SliderFloat(
				"Y Pos",
				&pos.y,
				-1000.f,
				1000.f
		);
		ImGui::SliderFloat(
				"Z Pos",
				&pos.z,
				-1000.f,
				1000.f
		);
		ImGui::End();

		// IMGUI END NEW FRAME

		// Update model matrix

		ubo.view = glm::lookAt(
				{
						pos.x,
						pos.y,
						pos.z
				},
				glm::vec3{},
				{
						0,
						0,
						1.f
				}
		);

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		ubo.model = glm::rotate(
				glm::mat4(1.f),
				time * glm::radians(input),
				glm::vec3(
						x,
						y,
						z
				));

		memcpy(
				uniformBufferMemPtrs[currentFrame],
				&ubo,
				sizeof(ubo)
		);

		// end update model matrix

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
				clearValues,
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

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext(imguiContext);

	glfwTerminate();
	return 0;
}

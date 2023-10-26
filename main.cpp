#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

// KEEP VULKAN INCLUDED AT THE TOP OR THINGS WILL BREAK
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_to_string.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <glm/ext.hpp>

#include "tiny_obj_loader.h"

#include <chrono>
#include <string>

#include "Vertex.h"
#include "Util.h"
#include "Frame.h"
#include "VkErrorHandling.h"
#include "Texture.h"

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

std::tuple<std::vector<Vertex>, std::vector<uint16_t>> loadObj(const char *filename) {
	tinyobj::ObjReaderConfig config{};
	config.triangulate = true;
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
	std::vector<uint16_t> indices;

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

	int vertexIndex = 0;
	for (int texIndex = 0; texIndex < static_cast<int>(attrib.texcoords.size()); texIndex += 2) {
		vertices[vertexIndex].texCoords[0] = attrib.texcoords[texIndex];
		vertices[vertexIndex].texCoords[1] = attrib.texcoords[texIndex + 1];
		vertexIndex++;
	}

	try {
		for (const auto &shape: shapes) {
			for (const auto &index: shape.mesh.indices) {
				indices.push_back(static_cast<uint16_t>(index.vertex_index));
			}
		}
	} catch (const std::exception &err) {
		std::cout << err.what() << std::endl;
	}


	return {
			std::move(vertices),
			std::move(indices)
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
			1600,
			900,
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
			vk::ImageType::e2D,
			vk::MemoryPropertyFlagBits::eDeviceLocal
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
	std::vector<uint16_t> indices{};
	std::tie(
			vertices,
			indices
	) = loadObj("models/monkey.obj");

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
			1600.f / 900.f,
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

	// BEGIN TEXTURES

	Texture texture(
			device,
			physicalDevice,
			commandBuffers[0],
			queue,
			"res/monkey.png"
	);

	// END TEXTURES

	// DESCRIPTOR SETS

	auto descriptorPool = util::createDescriptorPool(
			device
	);

	auto descriptorSets = util::createDescriptorSets(
			device,
			descriptorPool,
			descriptorSetLayout,
			1
	);

	std::vector<vk::DescriptorSet> drawDescriptorSets;
	std::for_each(
			descriptorSets.begin(),
			descriptorSets.end(),
			[&drawDescriptorSets](vk::raii::DescriptorSet &descriptorSet) mutable {
				std::cout << "descriptor set: " << *descriptorSet << std::endl;
				drawDescriptorSets.push_back(*descriptorSet);
			}
	);
	std::cout << "Descriptor set count " << drawDescriptorSets.size() << std::endl;

	std::array<vk::WriteDescriptorSet, 3> writeDescriptorSets;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::DescriptorBufferInfo descriptorBufferInfo{};
		descriptorBufferInfo.setOffset(0);
		descriptorBufferInfo.setBuffer(*uniformBuffers[i]);
		descriptorBufferInfo.setRange(sizeof(UniformBufferObject));

		writeDescriptorSets[i].setDstSet(*descriptorSets[0]);
		writeDescriptorSets[i].setDstBinding(0);
		writeDescriptorSets[i].setDstArrayElement(0);
		writeDescriptorSets[i].setDescriptorType(vk::DescriptorType::eUniformBuffer);
		writeDescriptorSets[i].setDescriptorCount(1);
		writeDescriptorSets[i].setBufferInfo(descriptorBufferInfo);
	}

	vk::DescriptorImageInfo imageDescriptorInfo{};
	imageDescriptorInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	imageDescriptorInfo.setSampler(texture.getSampler());
	imageDescriptorInfo.setImageView(texture.getImageView());

	writeDescriptorSets[2].setDstSet(*descriptorSets[0]);
	writeDescriptorSets[2].setDstBinding(1);
	writeDescriptorSets[2].setDstArrayElement(0);
	writeDescriptorSets[2].setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	writeDescriptorSets[2].setDescriptorCount(1);
	writeDescriptorSets[2].setImageInfo(imageDescriptorInfo);

	device.updateDescriptorSets(
			writeDescriptorSets,
			nullptr
	);

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

	// BEGIN SYNCHRONIZATION 

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

	// END SYNCHRONIZATION

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

	struct ModelSettings {
		struct {
			float x, y, z;
		} pos{
				0.f,
				5.f,
				2.f
		};
		struct {
			float x, y, z;
		} rotation{};
	} modelSettings{};

	commandPool.reset();

	auto lastTime = std::chrono::high_resolution_clock::now();

	while (!glfwWindowShouldClose(window)) {
		auto currentTime = std::chrono::high_resolution_clock::now();
		float delta = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
		lastTime = currentTime;

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
		ImGui::Text("Model Rotation");
		ImGui::SliderFloat(
				"World X",
				&modelSettings.rotation.x,
				-360.f,
				360.f
		);
		ImGui::SliderFloat(
				"World Y",
				&modelSettings.rotation.y,
				-360.f,
				360.f
		);
		ImGui::SliderFloat(
				"World Z",
				&modelSettings.rotation.z,
				-360.f,
				360.f
		);
		ImGui::Text("Camera Position");
		ImGui::SliderFloat(
				"Camera X",
				&modelSettings.pos.x,
				-1000.f,
				1000.f
		);
		ImGui::SliderFloat(
				"Camera Y",
				&modelSettings.pos.y,
				-1000.f,
				1000.f
		);
		ImGui::SliderFloat(
				"Camera Z",
				&modelSettings.pos.z,
				-1000.f,
				1000.f
		);
		bool resetPressed = ImGui::Button(
				"Reset",
				{
						100.f,
						25.f
				}
		);
		ImGui::End();

		// IMGUI END NEW FRAME

		// Update model matrix

		ubo.view = glm::lookAt(
				{
						modelSettings.pos.x,
						modelSettings.pos.y,
						modelSettings.pos.z
				},
				glm::vec3{},
				{
						0,
						0,
						1.f
				}
		);

		if (resetPressed) {
			ubo.model = glm::identity<glm::mat4>();
			modelSettings.rotation.x = 0;
			modelSettings.rotation.y = 0;
			modelSettings.rotation.z = 0;
		} else {
			glm::vec3 xRot = glm::vec3(
					1.f,
					0.f,
					0.f
			) * glm::radians(modelSettings.rotation.x);
			glm::vec3 yRot = glm::vec3(
					0.f,
					1.f,
					0.f
			) * glm::radians(modelSettings.rotation.y);
			glm::vec3 zRot = glm::vec3(
					0.f,
					0.f,
					1.f
			) * glm::radians(modelSettings.rotation.z);
			glm::vec3 finalRot = xRot + yRot + zRot;
			if (glm::length(finalRot) > 0.f) {
				ubo.model = glm::rotate(
						glm::identity<glm::mat4>(),
						glm::length(finalRot),
						glm::normalize(finalRot));
			} else {
				ubo.model = glm::mat4(1.f);
			}
		}

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
				drawDescriptorSets
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

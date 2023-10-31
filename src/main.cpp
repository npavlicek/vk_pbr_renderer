#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

// KEEP VULKAN INCLUDED AT THE TOP OR THINGS WILL BREAK
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_to_string.hpp>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"

#include <glm/ext.hpp>

#include "tiny_obj_loader.h"

#include <chrono>
#include <string>

#include "Vertex.h"
#include "Util.h"
#include "Frame.h"
#include "VkErrorHandling.h"
#include "Texture.h"
#include "VulkanManager.h"

int currentFrame = 0;

struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

void keyCallback(
	GLFWwindow *window,
	int key,
	int scancode,
	int action,
	int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(
			window,
			GLFW_TRUE);
	}
}

std::tuple<std::vector<Vertex>, std::vector<uint16_t>> loadObj(const char *filename)
{
	tinyobj::ObjReaderConfig config{};
	config.triangulate = true;
	tinyobj::ObjReader objReader{};
	objReader.ParseFromFile(
		filename,
		config);

	const auto attrib = objReader.GetAttrib();
	const auto shapes = objReader.GetShapes();
	// const std::vector<tinyobj::material_t> &materials = objReader.GetMaterials();

	std::srand(std::time(nullptr));

	std::vector<Vertex> vertices{};
	std::vector<uint16_t> indices{};

	for (int index = 0; index < static_cast<int>(shapes.at(0).mesh.indices.size()); index++)
	{
		const auto &idx = shapes.at(0).mesh.indices.at(index);

		Vertex vertex;
		vertex.pos[0] = attrib.vertices.at(3 * idx.vertex_index);
		vertex.pos[1] = attrib.vertices.at(3 * idx.vertex_index + 1);
		vertex.pos[2] = attrib.vertices.at(3 * idx.vertex_index + 2);
		vertex.color[0] = static_cast<float>(std::rand()) / RAND_MAX;
		vertex.color[1] = static_cast<float>(std::rand()) / RAND_MAX;
		vertex.color[2] = static_cast<float>(std::rand()) / RAND_MAX;
		vertex.texCoords[0] = attrib.texcoords.at(2 * idx.texcoord_index);
		// REMEMBER THE Y AXIS IS FLIPPED IN VULKAN
		vertex.texCoords[1] = 1 - attrib.texcoords.at(2 * idx.texcoord_index + 1);

		indices.push_back(index);
		vertices.push_back(vertex);
	}

	return std::make_tuple(
		vertices,
		indices);
}

int main()
{
	if (!glfwInit())
		throw std::runtime_error("Could not initialize GLFW");

	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow *window = glfwCreateWindow(
		1600,
		900,
		"Testing Vulkan!",
		nullptr,
		nullptr);

	if (!window)
	{
		std::cerr << "Could not open primary window!" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(
		window,
		keyCallback);

	VkResCheck res;

	VulkanManager vulkanManager(window, 2);

	// BEGIN DEPTH IMAGE

	vk::Format depthImageFormat = util::selectDepthFormat(*vulkanManager.getVulkanState().physicalDevice.get());

	auto surfaceCapabilities = vulkanManager.getVulkanState().surfaceCapabilities;

	std::vector<vk::raii::DeviceMemory> depthImageMemorys;
	std::vector<vk::raii::Image> depthImages;
	std::vector<vk::Image> depthImagesTemp;

	// TODO: MODIFY THE RANGE OF THIS LOOP TO MATCH HOWEVER MANY IMAGES WE GET ON THE SWAPCHAIN
	for (int i = 0; i < 3; i++)
	{
		vk::raii::DeviceMemory depthImageMemory{nullptr};
		vk::raii::Image depthImage{nullptr};
		std::tie(
			depthImage,
			depthImageMemory) = util::createImage(*vulkanManager.getVulkanState().device.get(), *vulkanManager.getVulkanState().physicalDevice.get(), vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, depthImageFormat, {surfaceCapabilities.currentExtent.width, surfaceCapabilities.currentExtent.height, 1}, vk::ImageType::e2D, vk::MemoryPropertyFlagBits::eDeviceLocal);
		depthImageMemorys.push_back(std::move(depthImageMemory));
		depthImagesTemp.push_back(*depthImage);
		depthImages.push_back(std::move(depthImage));
	}

	std::vector<vk::raii::ImageView> depthImageViews = util::createImageViews(
		*vulkanManager.getVulkanState().device.get(),
		depthImagesTemp,
		depthImageFormat,
		vk::ImageAspectFlagBits::eDepth);

	depthImagesTemp.clear();

	// END DEPTH IMAGE

	auto renderPass = util::createRenderPass(
		*vulkanManager.getVulkanState().device.get(),
		vulkanManager.getVulkanState().swapChainFormat,
		depthImageFormat);

	vk::raii::Pipeline pipeline{nullptr};
	vk::raii::PipelineLayout pipelineLayout{nullptr};
	vk::raii::PipelineCache pipelineCache{nullptr};
	vk::raii::DescriptorSetLayout descriptorSetLayout{nullptr};
	std::tie(
		pipeline,
		pipelineLayout,
		pipelineCache,
		descriptorSetLayout) = util::createPipeline(*vulkanManager.getVulkanState().device.get(), renderPass, *vulkanManager.getVulkanState().shaderModules.get());
	auto queue = vulkanManager.getVulkanState().device.get()->getQueue(
		vulkanManager.getVulkanState().queueFamilyGraphicsIndex,
		0);

	std::cout << "NUMBER OF IMAGES: " << vulkanManager.getVulkanState().swapChainImageViews.get()->size() << std::endl;

	auto frameBuffers = util::createFrameBuffers(
		*vulkanManager.getVulkanState().device.get(),
		renderPass,
		*vulkanManager.getVulkanState().swapChainImageViews.get(),
		depthImageViews,
		vulkanManager.getVulkanState().surfaceCapabilities);

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	auto objLoadStartTime = std::chrono::high_resolution_clock::now();
	std::tie(
		vertices,
		indices) = loadObj("models/monkey.obj");
	auto objLoadEndTime = std::chrono::high_resolution_clock::now();

	auto elapsedTime = static_cast<std::chrono::duration<float, std::chrono::milliseconds::period>>(objLoadEndTime -
																									objLoadStartTime);

	std::cout << "Took " << elapsedTime << " milliseconds to load OBJ model" << std::endl;

	// Create staging and vertex buffers and upload data
	auto physicalDeviceMemoryProperties = vulkanManager.getVulkanState().physicalDevice.get()->getMemoryProperties();
	const auto vertexBufferSize = sizeof(vertices[0]) * vertices.size();

	vk::raii::Buffer stagingBuffer{nullptr};
	vk::raii::DeviceMemory stagingBufferMemory{nullptr};
	std::tie(
		stagingBuffer,
		stagingBufferMemory) = util::createBuffer(*vulkanManager.getVulkanState().device.get(),
												  physicalDeviceMemoryProperties,
												  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
												  vertexBufferSize,
												  vk::BufferUsageFlagBits::eTransferSrc);

	vk::raii::Buffer vertexBuffer{nullptr};
	vk::raii::DeviceMemory vertexBufferMemory{nullptr};
	std::tie(
		vertexBuffer,
		vertexBufferMemory) = util::createBuffer(*vulkanManager.getVulkanState().device.get(),
												 physicalDeviceMemoryProperties,
												 vk::MemoryPropertyFlagBits::eDeviceLocal,
												 vertexBufferSize,
												 vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);

	util::uploadVertexData(
		*vulkanManager.getVulkanState().device.get(),
		stagingBufferMemory,
		vertices);
	//

	// Create index buffers and staging buffers and upload index data
	const auto indexBufferSize = sizeof(indices[0]) * indices.size();

	vk::raii::Buffer indexBufferStaging{nullptr};
	vk::raii::DeviceMemory indexBufferStagingMemory{nullptr};
	std::tie(
		indexBufferStaging,
		indexBufferStagingMemory) = util::createBuffer(*vulkanManager.getVulkanState().device.get(),
													   physicalDeviceMemoryProperties,
													   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
													   indexBufferSize,
													   vk::BufferUsageFlagBits::eTransferSrc);

	vk::raii::Buffer indexBuffer{nullptr};
	vk::raii::DeviceMemory indexBufferMemory{nullptr};
	std::tie(
		indexBuffer,
		indexBufferMemory) = util::createBuffer(*vulkanManager.getVulkanState().device.get(),
												physicalDeviceMemoryProperties,
												vk::MemoryPropertyFlagBits::eDeviceLocal,
												indexBufferSize,
												vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer);

	util::uploadIndexData(
		*vulkanManager.getVulkanState().device.get(),
		indexBufferStagingMemory,
		indices);
	//

	// Copy staging buffer data to vertex buffer
	{
		vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.setCommandBufferCount(1);
		commandBufferAllocateInfo.setCommandPool(**vulkanManager.getVulkanState().commandPool.get());
		commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);
		auto commandBuffer = std::move(
			vk::raii::CommandBuffers{
				*vulkanManager.getVulkanState().device.get(),
				commandBufferAllocateInfo}[0]);

		vk::CommandBufferBeginInfo commandBufferBeginInfo;
		commandBufferBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer.begin(commandBufferBeginInfo);

		vk::BufferCopy bufferCopy;
		bufferCopy.setSize(vertexBufferSize);
		commandBuffer.copyBuffer(
			*stagingBuffer,
			*vertexBuffer,
			bufferCopy);

		commandBuffer.end();

		vk::SubmitInfo submitInfo;
		submitInfo.setCommandBufferCount(1);
		submitInfo.setCommandBuffers(*commandBuffer);

		queue.submit(submitInfo);

		vulkanManager.getVulkanState().device.get()->waitIdle();
	}
	//

	// Copy staging buffer data to index buffer
	{
		vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.setCommandBufferCount(1);
		commandBufferAllocateInfo.setCommandPool(**vulkanManager.getVulkanState().commandPool.get());
		commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);
		auto commandBuffer = std::move(
			vk::raii::CommandBuffers{
				*vulkanManager.getVulkanState().device.get(),
				commandBufferAllocateInfo}[0]);

		vk::CommandBufferBeginInfo commandBufferBeginInfo;
		commandBufferBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer.begin(commandBufferBeginInfo);

		vk::BufferCopy bufferCopy;
		bufferCopy.setSize(indexBufferSize);
		commandBuffer.copyBuffer(
			*indexBufferStaging,
			*indexBuffer,
			bufferCopy);

		commandBuffer.end();

		vk::SubmitInfo submitInfo;
		submitInfo.setCommandBufferCount(1);
		submitInfo.setCommandBuffers(*commandBuffer);

		queue.submit(submitInfo);

		vulkanManager.getVulkanState().device.get()->waitIdle();
	}
	//

	// UNIFORM BUFFERS

	void *uniformBufferMemPtrs[vulkanManager.getVulkanState().maxFramesInFlight];
	std::vector<vk::raii::Buffer> uniformBuffers;
	std::vector<vk::raii::DeviceMemory> uniformBufferMemory;
	for (int i = 0; i < vulkanManager.getVulkanState().maxFramesInFlight; i++)
	{
		auto uniformBuffer = util::createBuffer(
			*vulkanManager.getVulkanState().device.get(),
			physicalDeviceMemoryProperties,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			sizeof(UniformBufferObject),
			vk::BufferUsageFlagBits::eUniformBuffer);
		uniformBuffers.push_back(std::move(std::get<vk::raii::Buffer>(uniformBuffer)));
		uniformBufferMemory.push_back(std::move(std::get<vk::raii::DeviceMemory>(uniformBuffer)));

		vkMapMemory(
			**vulkanManager.getVulkanState().device.get(),
			*uniformBufferMemory[i],
			0,
			sizeof(UniformBufferObject),
			0,
			&uniformBufferMemPtrs[i]);
	}

	UniformBufferObject ubo{};
	ubo.projection = glm::perspective(
		glm::radians(45.f),
		1600.f / 900.f,
		0.1f,
		2000.f);
	ubo.view = glm::lookAt(
		glm::vec3{
			0.f,
			0.f,
			5.f},
		glm::vec3{
			0.f,
			0.f,
			0.f},
		glm::vec3{
			0.f,
			-1.f,
			0.f});
	ubo.model = glm::identity<glm::mat4>();

	for (const auto &uniformBufferMemPtr : uniformBufferMemPtrs)
	{
		memcpy(
			uniformBufferMemPtr,
			&ubo,
			sizeof(ubo));
	}

	// END UNIFORM BUFFERS

	// BEGIN TEXTURES

	Texture texture(
		*vulkanManager.getVulkanState().device.get(),
		*vulkanManager.getVulkanState().physicalDevice.get(),
		(*vulkanManager.getVulkanState().commandBuffers.get())[0],
		queue,
		"res/monkey.png");

	// END TEXTURES

	// DESCRIPTOR SETS

	auto descriptorPool = util::createDescriptorPool(
		*vulkanManager.getVulkanState().device.get());

	auto descriptorSets = util::createDescriptorSets(
		*vulkanManager.getVulkanState().device.get(),
		descriptorPool,
		descriptorSetLayout,
		1);

	std::vector<vk::DescriptorSet> drawDescriptorSets;
	std::for_each(
		descriptorSets.begin(),
		descriptorSets.end(),
		[&drawDescriptorSets](vk::raii::DescriptorSet &descriptorSet) mutable
		{
			drawDescriptorSets.push_back(*descriptorSet);
		});

	std::array<vk::WriteDescriptorSet, 3> writeDescriptorSets;

	for (int i = 0; i < vulkanManager.getVulkanState().maxFramesInFlight; i++)
	{
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

	vulkanManager.getVulkanState().device.get()->updateDescriptorSets(
		writeDescriptorSets,
		nullptr);

	// END DESCRIPTOR SETS

	// BEGIN IMGUI

	auto imguiContext = ImGui::CreateContext();
	ImGui_ImplGlfw_InitForVulkan(
		window,
		true);

	ImGui_ImplVulkan_InitInfo imGuiImplVulkanInitInfo{
		**vulkanManager.getVulkanState().instance.get(),
		**vulkanManager.getVulkanState().physicalDevice.get(),
		**vulkanManager.getVulkanState().device.get(),
		static_cast<uint32_t>(vulkanManager.getVulkanState().queueFamilyGraphicsIndex),
		*queue,
		*pipelineCache,
		*descriptorPool,
		0,
		surfaceCapabilities.minImageCount,
		surfaceCapabilities.minImageCount,
		static_cast<VkSampleCountFlagBits>(vk::SampleCountFlagBits::e1),
		false,
		static_cast<VkFormat>(vulkanManager.getVulkanState().swapChainFormat.format),
		nullptr,
		nullptr};

	ImGui_ImplVulkan_Init(
		&imGuiImplVulkanInitInfo,
		*renderPass);

	// UPLOAD IMGUI FONTS
	{
		vk::raii::CommandBuffer commandBuffer = std::move(
			util::createCommandBuffers(
				*vulkanManager.getVulkanState().device.get(),
				*vulkanManager.getVulkanState().commandPool.get(),
				1)[0]);

		vk::CommandBufferBeginInfo commandBufferBeginInfo;
		commandBufferBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer.begin(commandBufferBeginInfo);

		ImGui_ImplVulkan_CreateFontsTexture(*commandBuffer);

		commandBuffer.end();
		vk::SubmitInfo submitInfo;
		submitInfo.setCommandBufferCount(1);
		submitInfo.setCommandBuffers(*commandBuffer);
		queue.submit(submitInfo);

		vulkanManager.getVulkanState().device.get()->waitIdle();
	}

	ImGui_ImplVulkan_DestroyFontUploadObjects();

	// END IMGUI

	// BEGIN SYNCHRONIZATION

	std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
	std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
	imageAvailableSemaphores.reserve(vulkanManager.getVulkanState().maxFramesInFlight);
	renderFinishedSemaphores.reserve(vulkanManager.getVulkanState().maxFramesInFlight);
	for (int i = 0; i < vulkanManager.getVulkanState().maxFramesInFlight; i++)
	{
		imageAvailableSemaphores.push_back(vulkanManager.getVulkanState().device->createSemaphore({}));
		renderFinishedSemaphores.push_back(vulkanManager.getVulkanState().device->createSemaphore({}));
	}

	std::vector<vk::raii::Fence> inFlightFences;
	inFlightFences.reserve(vulkanManager.getVulkanState().maxFramesInFlight);
	for (int i = 0; i < vulkanManager.getVulkanState().maxFramesInFlight; i++)
	{
		inFlightFences.push_back(vulkanManager.getVulkanState().device->createFence({vk::FenceCreateFlagBits::eSignaled}));
	}

	// END SYNCHRONIZATION

	vk::ClearColorValue clearColorValue{
		0.f,
		0.f,
		0.f,
		1.f};
	vk::ClearDepthStencilValue clearDepthValue{
		1.f,
		0};
	std::vector<vk::ClearValue> clearValues{
		clearColorValue,
		clearDepthValue};
	vk::Rect2D renderArea{
		{0,
		 0},
		surfaceCapabilities.currentExtent};

	struct ModelSettings
	{
		struct
		{
			float x, y, z;
		} pos{
			0.f,
			5.f,
			2.f};
		struct
		{
			float x, y, z;
		} rotation{};
	} modelSettings{};

	vulkanManager.getVulkanState().commandPool->reset();

	auto lastTime = std::chrono::high_resolution_clock::now();

	while (!glfwWindowShouldClose(window))
	{
		auto currentTime = std::chrono::high_resolution_clock::now();
		float delta = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
		lastTime = currentTime;

		glfwPollEvents();

		res = vulkanManager.getVulkanState().device->waitForFences(
			*inFlightFences[currentFrame],
			VK_TRUE,
			UINT32_MAX);
		vulkanManager.getVulkanState().device->resetFences(*inFlightFences[currentFrame]);

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
			360.f);
		ImGui::SliderFloat(
			"World Y",
			&modelSettings.rotation.y,
			-360.f,
			360.f);
		ImGui::SliderFloat(
			"World Z",
			&modelSettings.rotation.z,
			-360.f,
			360.f);
		ImGui::Text("Camera Position");
		ImGui::SliderFloat(
			"Camera X",
			&modelSettings.pos.x,
			-50.f,
			50.f);
		ImGui::SliderFloat(
			"Camera Y",
			&modelSettings.pos.y,
			-50.f,
			50.f);
		ImGui::SliderFloat(
			"Camera Z",
			&modelSettings.pos.z,
			-50.f,
			50.f);
		bool resetPressed = ImGui::Button(
			"Reset",
			{100.f,
			 25.f});
		ImGui::End();

		// IMGUI END NEW FRAME

		// Update model matrix

		ubo.view = glm::lookAt(
			{modelSettings.pos.x,
			 modelSettings.pos.y,
			 modelSettings.pos.z},
			glm::vec3{},
			{0,
			 -1.f,
			 0});

		if (resetPressed)
		{
			ubo.model = glm::identity<glm::mat4>();
			modelSettings.rotation.x = 0;
			modelSettings.rotation.y = 0;
			modelSettings.rotation.z = 0;
		}
		else
		{
			glm::vec3 xRot = glm::vec3(
								 1.f,
								 0.f,
								 0.f) *
							 glm::radians(modelSettings.rotation.x);
			glm::vec3 yRot = glm::vec3(
								 0.f,
								 1.f,
								 0.f) *
							 glm::radians(modelSettings.rotation.y);
			glm::vec3 zRot = glm::vec3(
								 0.f,
								 0.f,
								 1.f) *
							 glm::radians(modelSettings.rotation.z);
			glm::vec3 finalRot = xRot + yRot + zRot;
			if (glm::length(finalRot) > 0.f)
			{
				ubo.model = glm::rotate(
					glm::identity<glm::mat4>(),
					glm::length(finalRot),
					glm::normalize(finalRot));
			}
			else
			{
				ubo.model = glm::mat4(1.f);
			}
		}

		//		ubo.view = glm::mat4(1.f);
		//		ubo.projection = glm::mat4(1.f);
		//		ubo.model = glm::mat4(1.f);

		memcpy(
			uniformBufferMemPtrs[currentFrame],
			&ubo,
			sizeof(ubo));

		// end update model matrix

		uint32_t imageIndex;
		std::tie(
			res,
			imageIndex) = vulkanManager.getVulkanState().swapChain->acquireNextImage(UINT64_MAX,
																					 *imageAvailableSemaphores[currentFrame],
																					 VK_NULL_HANDLE);

		pbr::Frame::begin(
			(*vulkanManager.getVulkanState().commandBuffers.get())[currentFrame],
			pipeline,
			pipelineLayout,
			vertexBuffer,
			indexBuffer,
			drawDescriptorSets);
		pbr::Frame::beginRenderPass(
			(*vulkanManager.getVulkanState().commandBuffers.get())[currentFrame],
			renderPass,
			frameBuffers[imageIndex],
			clearValues,
			renderArea);
		pbr::Frame::draw(
			(*vulkanManager.getVulkanState().commandBuffers.get())[currentFrame],
			renderArea,
			static_cast<int>(indices.size()));
		pbr::Frame::endRenderPass((*vulkanManager.getVulkanState().commandBuffers.get())[currentFrame]);
		pbr::Frame::end((*vulkanManager.getVulkanState().commandBuffers.get())[currentFrame]);

		std::vector<vk::PipelineStageFlags> pipelineStageFlags{vk::PipelineStageFlagBits::eColorAttachmentOutput};

		vk::SubmitInfo submitInfo;
		submitInfo.setWaitSemaphoreCount(1);
		submitInfo.setWaitSemaphores(*imageAvailableSemaphores[currentFrame]);
		submitInfo.setWaitDstStageMask(pipelineStageFlags);
		submitInfo.setCommandBufferCount(1);
		submitInfo.setCommandBuffers(*(*vulkanManager.getVulkanState().commandBuffers.get())[currentFrame]);
		submitInfo.setSignalSemaphoreCount(1);
		submitInfo.setSignalSemaphores(*renderFinishedSemaphores[currentFrame]);

		queue.submit(
			submitInfo,
			*inFlightFences[currentFrame]);

		vk::PresentInfoKHR presentInfo;
		presentInfo.setSwapchainCount(1);
		presentInfo.setSwapchains(**vulkanManager.getVulkanState().swapChain.get());
		presentInfo.setImageIndices(imageIndex);
		presentInfo.setWaitSemaphoreCount(1);
		presentInfo.setWaitSemaphores(*renderFinishedSemaphores[currentFrame]);

		res = queue.presentKHR(presentInfo);

		currentFrame = (currentFrame + 1) % vulkanManager.getVulkanState().maxFramesInFlight;
	}
	vulkanManager.getVulkanState().device->waitIdle();

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext(imguiContext);

	glfwTerminate();
	return 0;
}

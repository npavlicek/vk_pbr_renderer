#include "Renderer.h"

Renderer::Renderer(GLFWwindow *window)
{
	this->window = window;

	context = vk::raii::Context{};

	vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{};
	debugUtilsMessengerCreateInfo.setMessageSeverity(
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
	debugUtilsMessengerCreateInfo.setMessageType(
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
	debugUtilsMessengerCreateInfo.setPfnUserCallback(&VkResCheck::PFN_vkDebugUtilsMessengerCallbackEXT);

	instance = util::createInstance(context, "Vulkan PBR Renderer", "Vulkan PBR Renderer", debugUtilsMessengerCreateInfo);

	debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfo);

	physicalDevice = util::selectPhysicalDevice(instance);
	queueFamilyGraphicsIndex = util::selectQueueFamily(physicalDevice);
	device = util::createDevice(physicalDevice, queueFamilyGraphicsIndex);
	queue = device.getQueue(queueFamilyGraphicsIndex, 0);
	commandPool = util::createCommandPool(device, queueFamilyGraphicsIndex);
	commandBuffers = util::createCommandBuffers(device, commandPool, framesInFlight);

	VmaAllocatorCreateInfo vmaCreateInfo{};
	vmaCreateInfo.device = *device;
	vmaCreateInfo.instance = *instance;
	vmaCreateInfo.physicalDevice = *physicalDevice;
	vmaCreateInfo.vulkanApiVersion = context.enumerateInstanceVersion();
	vmaCreateAllocator(&vmaCreateInfo, &vmaAllocator);

	surface = util::createSurface(instance, window);
	createSwapChain();

	createDepthBuffers();

	renderPass = util::createRenderPass(device, swapChainFormat, depthImageFormat);
	shaderModules = util::createShaderModules(device, "shaders/vert.spv", "shaders/frag.spv");
	std::tie(
		pipeline,
		pipelineLayout,
		pipelineCache,
		descriptorSetLayout) = util::createPipeline(device, renderPass, shaderModules);

	frameBuffers = util::createFrameBuffers(device, renderPass, swapChainImageViews, depthImageViews, swapChainSurfaceCapabilities);

	createUniformBuffers();
	createDescriptorObjects();
	createSyncObjects();
	initializeImGui();

	clearColorValue = vk::ClearColorValue{
		0.f,
		0.f,
		0.f,
		1.f};
	clearDepthValue = vk::ClearDepthStencilValue{
		1.f,
		0};
	clearValues.push_back(clearColorValue);
	clearValues.push_back(clearDepthValue);
	renderArea = vk::Rect2D{
		{0,
		 0},
		swapChainSurfaceCapabilities.currentExtent};

	ubo.model = glm::mat4(1.f);
	ubo.view = glm::mat4(1.f);
	ubo.projection = glm::perspective(
		45.f,
		swapChainSurfaceCapabilities.currentExtent.width * 1.f / swapChainSurfaceCapabilities.currentExtent.height,
		0.1f,
		100.f);
}

Renderer::~Renderer()
{
	device.waitIdle();

	(*device).freeDescriptorSets(*descriptorPool, descriptorSets);

	for (int i = 0; i < static_cast<int>(uniformBuffers.size()); i++)
	{
		vmaUnmapMemory(vmaAllocator, uniformBufferAllocations[i]);
		vmaDestroyBuffer(vmaAllocator, uniformBuffers[i], uniformBufferAllocations[i]);
	}

	vmaDestroyAllocator(vmaAllocator);
}

void Renderer::destroy()
{
	device.waitIdle();

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext(imGuiContext);

	glfwTerminate();
}

void Renderer::destroyModel(Model &model)
{
	device.waitIdle();

	model.destroy(vmaAllocator);
}

void Renderer::resetCommandBuffers()
{
	commandPool.reset();
}

void Renderer::render(const std::vector<Model> &models, glm::mat4 view)
{
	res = device.waitForFences(*inFlightFences[currentFrame], VK_TRUE, UINT32_MAX);
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

	ubo.view = glm::lookAt(glm::vec3(modelSettings.pos.x, modelSettings.pos.y, modelSettings.pos.z), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, -1.f, 0.f));

	uint32_t imageIndex;
	std::tie(
		res,
		imageIndex) = swapChain.acquireNextImage(UINT64_MAX, *imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE);

	pbr::Frame::begin(
		commandBuffers.at(currentFrame),
		pipeline,
		pipelineLayout,
		vertexBuffer,
		indexBuffer,
		descriptorSets);

	pbr::Frame::beginRenderPass(
		commandBuffers.at(currentFrame),
		renderPass,
		frameBuffers[imageIndex],
		clearValues,
		renderArea);

	vk::Viewport viewport{
		static_cast<float>(renderArea.offset.x),
		static_cast<float>(renderArea.extent.height),
		static_cast<float>(renderArea.extent.width),
		-static_cast<float>(renderArea.extent.height),
		0,
		1};

	commandBuffers[currentFrame].setScissor(0, renderArea);
	commandBuffers[currentFrame].setViewport(0, viewport);

	for (const auto &model : models)
	{
		ubo.model = model.getModel();
		ubo.view = view;
		vkCmdPushConstants(*commandBuffers[currentFrame], *pipelineLayout, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformData), &ubo);
		model.draw(*commandBuffers[currentFrame]);
	}

	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *commandBuffers[currentFrame]);

	pbr::Frame::endRenderPass(commandBuffers.at(currentFrame));
	pbr::Frame::end(commandBuffers.at(currentFrame));

	std::vector<vk::PipelineStageFlags> pipelineStageFlags{vk::PipelineStageFlagBits::eColorAttachmentOutput};

	vk::SubmitInfo submitInfo;
	submitInfo.setWaitSemaphoreCount(1);
	submitInfo.setWaitSemaphores(*imageAvailableSemaphores[currentFrame]);
	submitInfo.setWaitDstStageMask(pipelineStageFlags);
	submitInfo.setCommandBufferCount(1);
	submitInfo.setCommandBuffers(*commandBuffers.at(currentFrame));
	submitInfo.setSignalSemaphoreCount(1);
	submitInfo.setSignalSemaphores(*renderFinishedSemaphores[currentFrame]);

	queue.submit(
		submitInfo,
		*inFlightFences[currentFrame]);

	vk::PresentInfoKHR presentInfo;
	presentInfo.setSwapchainCount(1);
	presentInfo.setSwapchains(*swapChain);
	presentInfo.setImageIndices(imageIndex);
	presentInfo.setWaitSemaphoreCount(1);
	presentInfo.setWaitSemaphores(*renderFinishedSemaphores[currentFrame]);

	res = queue.presentKHR(presentInfo);

	currentFrame = (currentFrame + 1) % framesInFlight;
}

void Renderer::initializeImGui()
{
	imGuiContext = ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(window, true);

	ImGui_ImplVulkan_InitInfo imGuiImplVulkanInitInfo{
		*instance,
		*physicalDevice,
		*device,
		static_cast<uint32_t>(queueFamilyGraphicsIndex),
		*queue,
		*pipelineCache,
		*descriptorPool,
		0,
		swapChainSurfaceCapabilities.minImageCount,
		swapChainSurfaceCapabilities.minImageCount,
		static_cast<VkSampleCountFlagBits>(vk::SampleCountFlagBits::e1),
		false,
		static_cast<VkFormat>(swapChainFormat.format),
		nullptr,
		nullptr};

	ImGui_ImplVulkan_Init(&imGuiImplVulkanInitInfo, *renderPass);

	CommandBuffer::beginSTC(*commandBuffers[0]);
	ImGui_ImplVulkan_CreateFontsTexture(*commandBuffers[0]);
	CommandBuffer::endSTC(*commandBuffers[0], *queue);

	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

Texture Renderer::createTexture(const char *path)
{
	return Texture{
		device,
		physicalDevice,
		commandBuffers[0],
		queue,
		path};
}

void Renderer::updateDescriptorSets(const Texture &tex)
{
	std::array<vk::WriteDescriptorSet, 3> writeDescriptorSets;

	for (int i = 0; i < framesInFlight; i++)
	{
		vk::DescriptorBufferInfo descriptorBufferInfo;
		descriptorBufferInfo.setBuffer(uniformBuffers[i]);
		descriptorBufferInfo.setOffset(0);
		descriptorBufferInfo.setRange(sizeof(UniformData));

		writeDescriptorSets.at(i).setBufferInfo(descriptorBufferInfo);
		writeDescriptorSets.at(i).setDescriptorCount(1);
		writeDescriptorSets.at(i).setDescriptorType(vk::DescriptorType::eUniformBuffer);
		writeDescriptorSets.at(i).setDstArrayElement(0);
		writeDescriptorSets.at(i).setDstBinding(0);
		writeDescriptorSets.at(i).setDstSet(descriptorSets[0]);
	}

	vk::DescriptorImageInfo descriptorImageInfo;
	descriptorImageInfo.setImageView(tex.getImageView());
	descriptorImageInfo.setSampler(tex.getSampler());
	descriptorImageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

	writeDescriptorSets.at(2).setImageInfo(descriptorImageInfo);
	writeDescriptorSets.at(2).setDescriptorCount(1);
	writeDescriptorSets.at(2).setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	writeDescriptorSets.at(2).setDstArrayElement(0);
	writeDescriptorSets.at(2).setDstBinding(1);
	writeDescriptorSets.at(2).setDstSet(descriptorSets[0]);

	device.updateDescriptorSets(writeDescriptorSets, nullptr);
}

void Renderer::createDescriptorObjects()
{
	descriptorPool = util::createDescriptorPool(device);
	auto ds = util::createDescriptorSets(device, descriptorPool, descriptorSetLayout, 1);

	auto &descriptorSetRef = descriptorSets;

	std::for_each(ds.begin(), ds.end(),
				  [&descriptorSetRef](vk::raii::DescriptorSet &current) mutable
				  {
					  descriptorSetRef.push_back(current.release());
				  });
}

void Renderer::createSyncObjects()
{
	vk::FenceCreateInfo fenceCreateInfo;
	fenceCreateInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);

	for (int i = 0; i < framesInFlight; i++)
	{
		imageAvailableSemaphores.push_back(device.createSemaphore({}));
		renderFinishedSemaphores.push_back(device.createSemaphore({}));
		inFlightFences.push_back(device.createFence(fenceCreateInfo));
	}
}

void Renderer::uploadUniformData(const UniformData &uniformData, int frame)
{
	memcpy(uniformBufferPtr[frame], &uniformData, sizeof(UniformData));
}

void Renderer::createUniformBuffers()
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.size = sizeof(UniformData);
	bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	VmaAllocationCreateInfo allocationCreateInfo{};
	allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
	allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	for (int i = 0; i < framesInFlight; i++)
	{
		VkBuffer buffer;
		VmaAllocation allocation;
		void *bufferPtr;
		vmaCreateBuffer(vmaAllocator, &bufferCreateInfo, &allocationCreateInfo, &buffer, &allocation, nullptr);
		vmaMapMemory(vmaAllocator, allocation, &bufferPtr);

		uniformBuffers.push_back(buffer);
		uniformBufferPtr.push_back(bufferPtr);
		uniformBufferAllocations.push_back(allocation);
	}
}

void Renderer::createSwapChain()
{
	swapChainFormat = util::selectSwapChainFormat(physicalDevice, surface);
	swapChainSurfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
	std::tie(swapChain, swapChainCreateInfo) = util::createSwapChain(device, physicalDevice, surface, swapChainFormat, swapChainSurfaceCapabilities);
	swapChainImages = swapChain.getImages();
	swapChainImageViews = util::createImageViews(device, swapChainImages, swapChainFormat.format, vk::ImageAspectFlagBits::eColor);
}

void Renderer::createDepthBuffers()
{
	depthImageFormat = util::selectDepthFormat(physicalDevice);

	std::vector<vk::Image> depthImagesTemp;

	// TODO: MODIFY THE RANGE OF THIS LOOP TO MATCH HOWEVER MANY IMAGES WE GET ON THE SWAPCHAIN
	for (int i = 0; i < 3; i++)
	{
		vk::raii::DeviceMemory depthImageMemory{nullptr};
		vk::raii::Image depthImage{nullptr};
		std::tie(
			depthImage,
			depthImageMemory) = util::createImage(device,
												  physicalDevice,
												  vk::ImageTiling::eOptimal,
												  vk::ImageUsageFlagBits::eDepthStencilAttachment,
												  depthImageFormat,
												  {swapChainSurfaceCapabilities.currentExtent.width,
												   swapChainSurfaceCapabilities.currentExtent.height,
												   1},
												  vk::ImageType::e2D,
												  vk::MemoryPropertyFlagBits::eDeviceLocal);
		depthImageMemorys.push_back(std::move(depthImageMemory));
		depthImagesTemp.push_back(*depthImage);
		depthImages.push_back(std::move(depthImage));
	}

	depthImageViews = util::createImageViews(
		device,
		depthImagesTemp,
		depthImageFormat,
		vk::ImageAspectFlagBits::eDepth);
}

Model Renderer::createModel(const char *path)
{
	return Model(vmaAllocator, *queue, *commandBuffers[0], path);
}

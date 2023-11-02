#include "Renderer.h"

Renderer::Renderer(GLFWwindow *window)
{
	// TODO: determine based on surface capabilities
	int maxFramesInFlight = 2;

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
	commandBuffers = util::createCommandBuffers(device, commandPool, maxFramesInFlight);

	VmaAllocatorCreateInfo vmaCreateInfo;
	vmaCreateInfo.device = *device;
	vmaCreateInfo.instance = *instance;
	vmaCreateInfo.physicalDevice = *physicalDevice;
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
		depthImages.push_back(std::move(depthImage));
		depthImagesTemp.push_back(*depthImage);
	}

	depthImageViews = util::createImageViews(
		device,
		depthImagesTemp,
		depthImageFormat,
		vk::ImageAspectFlagBits::eDepth);
}

void Renderer::uploadVertexData(std::vector<Vertex> vertices)
{
	VkDeviceSize memorySize = vertices.size() * sizeof(Vertex);

	VkBufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.size = memorySize;

	VmaAllocationCreateInfo vmaAllocCreateInfo;
	vmaAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
	vmaAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;
	VmaAllocationInfo stagingBufferAllocInfo;
	vmaCreateBuffer(vmaAllocator, &bufferCreateInfo, &vmaAllocCreateInfo, &stagingBuffer, &stagingBufferAllocation, &stagingBufferAllocInfo);

	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	vmaAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	vmaAllocCreateInfo.flags = 0;

	VkBuffer vertexBuffer;
	VmaAllocation vertexBufferAllocation;
	VmaAllocationInfo vertexBufferAllocInfo;
	vmaCreateBuffer(vmaAllocator, &bufferCreateInfo, &vmaAllocCreateInfo, &vertexBuffer, &vertexBufferAllocation, &vertexBufferAllocInfo);

	memcpy(stagingBufferAllocInfo.pMappedData, vertices.data(), memorySize);

	CommandBuffer::beginSTC(commandBuffers[0]);

	vk::BufferCopy bufferCopy;
	bufferCopy.setSize(memorySize);

	commandBuffers[0].copyBuffer(stagingBuffer, vertexBuffer, bufferCopy);

	CommandBuffer::endSTC(commandBuffers[0], queue);
}

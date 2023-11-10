#include "Renderer.h"
#include "Model.h"
#include "Validation.h"
#include "CommandBuffer.h"
#include "VkErrorHandling.h"
#include "VkExt.h"

#include <chrono>
#include <stdint.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_structs.hpp>

namespace N
{
Renderer::Renderer(GLFWwindow *window)
{
	this->window = window;

	createInstance();
	selectPhysicalDevice();
	selectGraphicsQueue();
	createDevice();

	graphicsQueue = device.getQueue(graphicsQueueIndex, 0);

	createCommandPool();
	createCommandBuffers();

	createDescriptorPool();

	VmaAllocatorCreateInfo vmaCreateInfo{};
	vmaCreateInfo.device = device;
	vmaCreateInfo.instance = instance;
	vmaCreateInfo.physicalDevice = physicalDevice;
	vmaCreateInfo.vulkanApiVersion = vk::enumerateInstanceVersion();
	vmaCreateAllocator(&vmaCreateInfo, &vmaAllocator);

	glfwCreateWindowSurface(instance, window, nullptr, reinterpret_cast<VkSurfaceKHR *>(&surface));

	detectSampleCounts();
	selectDepthFormat();

	N::SwapChainCreateInfo swapChainCreateInfo{};
	swapChainCreateInfo.device = device;
	swapChainCreateInfo.physicalDevice = physicalDevice;
	swapChainCreateInfo.framesInFlight = framesInFlight;
	swapChainCreateInfo.surface = surface;
	swapChain.create(swapChainCreateInfo);

	N::RenderPassCreateInfo renderPassCreateInfo;
	renderPassCreateInfo.device = device;
	renderPassCreateInfo.surfaceFormat = swapChain.getSurfaceFormat().format;
	renderPassCreateInfo.depthFormat = depthFormat;
	renderPassCreateInfo.samples = samples;
	renderPass.create(renderPassCreateInfo);

	N::PBRPipelineCreateInfo pipelineCreateInfo;
	pipelineCreateInfo.device = device;
	pipelineCreateInfo.renderPass = renderPass.get();
	pipelineCreateInfo.samples = samples;
	pipeline.create(pipelineCreateInfo);

	createDepthObjects();
	createRenderTargets();
	createFrameBuffers();
	createSyncObjects();
	initializeImGui();

	sampler = Material::createSampler(device, physicalDevice.getProperties().limits.maxSamplerAnisotropy);

	vk::Extent2D extent = physicalDevice.getSurfaceCapabilitiesKHR(surface).currentExtent;

	clearColorValue = vk::ClearColorValue{0.f, 0.f, 0.f, 1.f};
	clearDepthValue = vk::ClearDepthStencilValue{1.f, 0};
	clearValues.push_back(clearColorValue);
	clearValues.push_back(clearDepthValue);

	mvpPushConstant.model = glm::mat4(1.f);
	mvpPushConstant.view = glm::mat4(1.f);
	mvpPushConstant.projection = glm::perspective(45.f, extent.width * 1.f / extent.height, 0.1f, 100.f);

	commandBuffers.at(0).reset({});
}

void Renderer::createInstance()
{
	std::vector<const char *> enabledLayers{};
	std::vector<const char *> enabledExtensions{};

#ifdef ENABLE_VULKAN_VALIDATION_LAYERS
	enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
	enabledLayers.push_back("VK_LAYER_LUNARG_monitor");
	Validation::areLayersAvailable(enabledLayers);

	vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{};
	debugUtilsMessengerCreateInfo.setMessageSeverity(
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
	debugUtilsMessengerCreateInfo.setMessageType(
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
	debugUtilsMessengerCreateInfo.setPfnUserCallback(&VkResCheck::PFN_vkDebugUtilsMessengerCallbackEXT);

	enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	// Add GLFW necessary extensions
	uint32_t glfwExtensionCount = 0;
	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	enabledExtensions.reserve(glfwExtensionCount);
	for (int i = 0; i < static_cast<int>(glfwExtensionCount); i++)
	{
		enabledExtensions.push_back(glfwExtensions[i]);
	}

	vk::ApplicationInfo applicationInfo("VulkanEngine", 1, "VulkanEngine", 1, vk::ApiVersion13);

	vk::InstanceCreateInfo instanceCreateInfo{};
#ifdef ENABLE_VULKAN_VALIDATION_LAYERS
	instanceCreateInfo.setPNext(&debugUtilsMessengerCreateInfo);
#endif
	instanceCreateInfo.setPApplicationInfo(&applicationInfo);
	instanceCreateInfo.setEnabledLayerCount(enabledLayers.size());
	instanceCreateInfo.setPEnabledLayerNames(enabledLayers);
	instanceCreateInfo.setEnabledExtensionCount(enabledExtensions.size());
	instanceCreateInfo.setPEnabledExtensionNames(enabledExtensions);

	instance = vk::createInstance(instanceCreateInfo);

#ifdef ENABLE_VULKAN_VALIDATION_LAYERS
	debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfo);
#endif
}

Renderer::~Renderer()
{
	device.waitIdle();

	device.resetDescriptorPool(descriptorPool);
	device.destroyDescriptorPool(descriptorPool);

	device.destroySampler(sampler);

	for (int i = 0; i < framesInFlight; i++)
	{
		device.destroyFramebuffer(frameBuffers.at(i));

		device.destroyImageView(depthImages.at(i).imageView);
		vmaDestroyImage(vmaAllocator, depthImages.at(i).image, depthImages.at(i).imageAllocation);

		device.destroyImageView(renderTargets.at(i).imageView);
		vmaDestroyImage(vmaAllocator, renderTargets.at(i).image, renderTargets.at(i).imageAllocation);

		device.destroySemaphore(imageAvailableSemaphores[i]);
		device.destroySemaphore(renderFinishedSemaphores[i]);
		device.destroyFence(inFlightFences[i]);
	}

	device.freeCommandBuffers(commandPool, commandBuffers);

	vmaDestroyAllocator(vmaAllocator);

	device.destroyCommandPool(commandPool);

	swapChain.destroy(device);
	pipeline.destroy(device);
	renderPass.destroy(device);

	device.destroy();

	instance.destroySurfaceKHR(surface);

#ifdef ENABLE_VULKAN_VALIDATION_LAYERS
	instance.destroyDebugUtilsMessengerEXT(debugMessenger);
#endif
	instance.destroy();
}

void Renderer::selectDepthFormat()
{
	std::vector<vk::Format> requestedFormats{vk::Format::eD16Unorm, vk::Format::eD32Sfloat};

	bool supported = true;
	for (const auto &format : requestedFormats)
	{
		if (!(physicalDevice.getFormatProperties(format).optimalTilingFeatures &
			  vk::FormatFeatureFlagBits::eDepthStencilAttachment))
		{
			supported = false;
		}
	}

	if (!supported)
	{
		throw std::runtime_error("requested depth buffer format not supported!");
	}

	depthFormat = vk::Format::eD32Sfloat;
}

void Renderer::createDescriptorPool()
{
	std::vector<vk::DescriptorPoolSize> poolSizes = {{vk::DescriptorType::eUniformBuffer, 100},
													 {vk::DescriptorType::eCombinedImageSampler, 100}};

	vk::DescriptorPoolCreateInfo createInfo;
	createInfo.setMaxSets(100);
	createInfo.setPoolSizeCount(poolSizes.size());
	createInfo.setPoolSizes(poolSizes);

	descriptorPool = device.createDescriptorPool(createInfo);
}

void Renderer::createCommandBuffers()
{
	vk::CommandBufferAllocateInfo createInfo;
	createInfo.setLevel(vk::CommandBufferLevel::ePrimary);
	createInfo.setCommandPool(commandPool);
	createInfo.setCommandBufferCount(framesInFlight);

	commandBuffers = device.allocateCommandBuffers(createInfo);
}

void Renderer::createCommandPool()
{
	vk::CommandPoolCreateInfo commandPoolCreateInfo;
	commandPoolCreateInfo.setQueueFamilyIndex(graphicsQueueIndex);
	commandPoolCreateInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

	commandPool = device.createCommandPool(commandPoolCreateInfo);
}

void Renderer::createFrameBuffers()
{
	auto extent = physicalDevice.getSurfaceCapabilitiesKHR(surface).currentExtent;

	vk::FramebufferCreateInfo frameBufferCreateInfo;
	frameBufferCreateInfo.setRenderPass(renderPass.get());
	frameBufferCreateInfo.setLayers(1);
	frameBufferCreateInfo.setWidth(extent.width);
	frameBufferCreateInfo.setHeight(extent.height);

	for (int i = 0; i < framesInFlight; i++)
	{
		std::array<vk::ImageView, 3> attachments{renderTargets.at(i).imageView, depthImages.at(i).imageView,
												 swapChain.getImageViews().at(i)};

		frameBufferCreateInfo.setAttachmentCount(attachments.size());
		frameBufferCreateInfo.setAttachments(attachments);

		frameBuffers.push_back(device.createFramebuffer(frameBufferCreateInfo));
	}
}

// TODO: make this function more specific for what gpu features i need
void Renderer::selectPhysicalDevice()
{
	auto physicalDevices = instance.enumeratePhysicalDevices();

	for (const auto &cur : physicalDevices)
	{
		if (cur.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		{
			physicalDevice = cur;
			break;
		}
	}
}

void Renderer::createDevice()
{
	auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties()[graphicsQueueIndex];

	std::vector<float> queuePriorities(queueFamilyProperties.queueCount, 1.f);

	vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
	deviceQueueCreateInfo.setQueueFamilyIndex(graphicsQueueIndex);
	deviceQueueCreateInfo.setQueueCount(queueFamilyProperties.queueCount);
	deviceQueueCreateInfo.setQueuePriorities(queuePriorities);

	std::vector<const char *> enabledExtensions{"VK_KHR_swapchain"};

	vk::PhysicalDeviceFeatures physicalDeviceFeatures;
	physicalDeviceFeatures.setSamplerAnisotropy(vk::True);
	physicalDeviceFeatures.setSampleRateShading(vk::True);

	vk::DeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.setQueueCreateInfoCount(1);
	deviceCreateInfo.setQueueCreateInfos(deviceQueueCreateInfo);
	deviceCreateInfo.setEnabledExtensionCount(enabledExtensions.size());
	deviceCreateInfo.setPEnabledExtensionNames(enabledExtensions);
	deviceCreateInfo.setPEnabledFeatures(&physicalDeviceFeatures);

	device = physicalDevice.createDevice(deviceCreateInfo);
}

void Renderer::selectGraphicsQueue()
{
	auto queueFamilies = physicalDevice.getQueueFamilyProperties();

	for (size_t i = 0; i < queueFamilies.size(); i++)
	{
		if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)
		{
			graphicsQueueIndex = static_cast<int>(i);
			break;
		}
	}
}

void Renderer::detectSampleCounts()
{
	vk::SampleCountFlags colorSamples = physicalDevice.getProperties().limits.framebufferColorSampleCounts;
	vk::SampleCountFlags depthSamples = physicalDevice.getProperties().limits.framebufferDepthSampleCounts;
	vk::SampleCountFlags combined = colorSamples & depthSamples;

	for (int i = 64; i > 0; i >>= 1)
	{
		vk::SampleCountFlagBits current = static_cast<vk::SampleCountFlagBits>(i);
		if (combined & current)
		{
			samples = current;
			break;
		}
	}
}

void Renderer::destroy()
{
	device.waitIdle();

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext(imGuiContext);
}

void Renderer::destroyModel(Model &model)
{
	device.waitIdle();

	model.destroy(vmaAllocator, device, descriptorPool);
}

void Renderer::render(const std::vector<Model> &models, glm::vec3 cameraPos, glm::mat4 view)
{
	auto res = device.waitForFences(inFlightFences[currentFrame], VK_TRUE, UINT32_MAX);
	vk::resultCheck(res, "error encountered while waiting for fence!");
	device.resetFences(inFlightFences[currentFrame]);

	const vk::CommandBuffer &cb = commandBuffers[currentFrame];

	// IMGUI NEW FRAME

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Settings");
	ImGui::Text("Model Rotation");
	ImGui::SliderFloat("World X", &modelSettings.rotation.x, -360.f, 360.f);
	ImGui::SliderFloat("World Y", &modelSettings.rotation.y, -360.f, 360.f);
	ImGui::SliderFloat("World Z", &modelSettings.rotation.z, -360.f, 360.f);
	ImGui::Text("Camera Position");
	ImGui::SliderFloat("Camera X", &modelSettings.pos.x, -50.f, 50.f);
	ImGui::SliderFloat("Camera Y", &modelSettings.pos.y, -50.f, 50.f);
	ImGui::SliderFloat("Camera Z", &modelSettings.pos.z, -50.f, 50.f);
	bool resetPressed = ImGui::Button("Reset", {100.f, 25.f});
	ImGui::End();

	// IMGUI END NEW FRAME

	mvpPushConstant.view = glm::lookAt(glm::vec3(modelSettings.pos.x, modelSettings.pos.y, modelSettings.pos.z),
									   glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, -1.f, 0.f));

	auto swapChainRes =
		device.acquireNextImageKHR(swapChain.getSwapChain(), UINT64_MAX, imageAvailableSemaphores[currentFrame]);
	vk::resultCheck(swapChainRes.result, "Could not acquire the next swapchain image!");
	uint32_t imageIndex = swapChainRes.value;

	cb.reset();

	vk::CommandBufferBeginInfo cbBeginInfo{};
	cb.begin(cbBeginInfo);
	vk::resultCheck(res, "Could not begin the current command buffer!");

	cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getPipeline());

	const vk::Rect2D renderArea{{0, 0}, physicalDevice.getSurfaceCapabilitiesKHR(surface).currentExtent};

	vk::RenderPassBeginInfo rpInfo;
	rpInfo.setRenderPass(renderPass.get());
	rpInfo.setFramebuffer(frameBuffers[imageIndex]);
	rpInfo.setRenderArea(renderArea);
	rpInfo.setClearValues(clearValues);
	rpInfo.setClearValueCount(clearValues.size());
	cb.beginRenderPass(rpInfo, vk::SubpassContents::eInline);

	vk::Viewport viewport{static_cast<float>(renderArea.offset.x),
						  static_cast<float>(renderArea.extent.height),
						  static_cast<float>(renderArea.extent.width),
						  -static_cast<float>(renderArea.extent.height),
						  0,
						  1};

	cb.setScissor(0, renderArea);
	cb.setViewport(0, viewport);

	for (const auto &model : models)
	{
		mvpPushConstant.model = model.getModel();
		mvpPushConstant.view = view;
		vkCmdPushConstants(commandBuffers[currentFrame], pipeline.getPipelineLayout(),
						   VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(N::MVPPushConstant),
						   &mvpPushConstant);
		model.draw(cb, pipeline.getPipelineLayout());
	}

	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);

	cb.endRenderPass();
	cb.end();

	std::vector<vk::PipelineStageFlags> pipelineStageFlags{vk::PipelineStageFlagBits::eColorAttachmentOutput};

	vk::SubmitInfo submitInfo;
	submitInfo.setWaitSemaphoreCount(1);
	submitInfo.setWaitSemaphores(imageAvailableSemaphores[currentFrame]);
	submitInfo.setWaitDstStageMask(pipelineStageFlags);
	submitInfo.setCommandBufferCount(1);
	submitInfo.setCommandBuffers(cb);
	submitInfo.setSignalSemaphoreCount(1);
	submitInfo.setSignalSemaphores(renderFinishedSemaphores[currentFrame]);

	graphicsQueue.submit(submitInfo, inFlightFences[currentFrame]);

	vk::PresentInfoKHR presentInfo;
	presentInfo.setSwapchainCount(1);
	presentInfo.setSwapchains(swapChain.getSwapChain());
	presentInfo.setImageIndices(imageIndex);
	presentInfo.setWaitSemaphoreCount(1);
	presentInfo.setWaitSemaphores(renderFinishedSemaphores[currentFrame]);

	vk::resultCheck(graphicsQueue.presentKHR(presentInfo), "Could not present the swapchain image!");

	currentFrame = (currentFrame + 1) % framesInFlight;
}

void Renderer::initializeImGui()
{
	imGuiContext = ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(window, true);

	ImGui_ImplVulkan_InitInfo imGuiImplVulkanInitInfo{instance,
													  physicalDevice,
													  device,
													  static_cast<uint32_t>(graphicsQueueIndex),
													  graphicsQueue,
													  nullptr,
													  descriptorPool,
													  0,
													  static_cast<uint32_t>(framesInFlight),
													  static_cast<uint32_t>(framesInFlight),
													  static_cast<VkSampleCountFlagBits>(samples),
													  false,
													  static_cast<VkFormat>(swapChain.getSurfaceFormat().format),
													  nullptr,
													  nullptr};

	ImGui_ImplVulkan_Init(&imGuiImplVulkanInitInfo, renderPass.get());

	// FIXME: change the command buffers variable
	CommandBuffer::beginSTC(commandBuffers[0]);
	ImGui_ImplVulkan_CreateFontsTexture(commandBuffers[0]);
	CommandBuffer::endSTC(commandBuffers[0], graphicsQueue);

	ImGui_ImplVulkan_DestroyFontUploadObjects();
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

void Renderer::createDepthObjects()
{
	auto extent = physicalDevice.getSurfaceCapabilitiesKHR(surface).currentExtent;
	vk::ImageCreateInfo imageCreateInfo{};
	imageCreateInfo.setImageType(vk::ImageType::e2D);
	imageCreateInfo.setArrayLayers(1);
	imageCreateInfo.setExtent(vk::Extent3D{extent.width, extent.height, 1});
	imageCreateInfo.setFormat(depthFormat);
	imageCreateInfo.setInitialLayout(vk::ImageLayout::eUndefined);
	imageCreateInfo.setMipLevels(1);
	imageCreateInfo.setSamples(samples);
	imageCreateInfo.setSharingMode(vk::SharingMode::eExclusive);
	imageCreateInfo.setTiling(vk::ImageTiling::eOptimal);
	imageCreateInfo.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment);

	VmaAllocationCreateInfo allocationCreateInfo{};
	allocationCreateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocationCreateInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

	vk::ImageSubresourceRange subresource;
	subresource.setLevelCount(1);
	subresource.setLayerCount(1);
	subresource.setBaseArrayLayer(0);
	subresource.setBaseMipLevel(0);
	subresource.setAspectMask(vk::ImageAspectFlagBits::eDepth);

	vk::ImageViewCreateInfo imageViewCreateInfo;
	imageViewCreateInfo.setViewType(vk::ImageViewType::e2D);
	imageViewCreateInfo.setFormat(depthFormat);
	imageViewCreateInfo.setComponents(vk::ComponentMapping{});
	imageViewCreateInfo.setSubresourceRange(subresource);

	for (int i = 0; i < framesInFlight; i++)
	{
		ImageObject cur;

		vmaCreateImage(vmaAllocator, reinterpret_cast<VkImageCreateInfo *>(&imageCreateInfo), &allocationCreateInfo,
					   reinterpret_cast<VkImage *>(&cur.image), &cur.imageAllocation, &cur.imageAllocationInfo);

		imageViewCreateInfo.setImage(cur.image);
		cur.imageView = device.createImageView(imageViewCreateInfo);

		depthImages.push_back(cur);
	}
}

void Renderer::createRenderTargets()
{
	auto extent = physicalDevice.getSurfaceCapabilitiesKHR(surface).currentExtent;

	vk::ImageCreateInfo imageCreateInfo{};
	imageCreateInfo.setImageType(vk::ImageType::e2D);
	imageCreateInfo.setArrayLayers(1);
	imageCreateInfo.setExtent(vk::Extent3D{extent.width, extent.height, 1});
	imageCreateInfo.setFormat(swapChain.getSurfaceFormat().format);
	imageCreateInfo.setInitialLayout(vk::ImageLayout::eUndefined);
	imageCreateInfo.setMipLevels(1);
	imageCreateInfo.setSamples(samples);
	imageCreateInfo.setSharingMode(vk::SharingMode::eExclusive);
	imageCreateInfo.setTiling(vk::ImageTiling::eOptimal);
	imageCreateInfo.setUsage(vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment);

	VmaAllocationCreateInfo allocCreateInfo{};
	allocCreateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocCreateInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

	vk::ImageSubresourceRange subresource;
	subresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
	subresource.setBaseArrayLayer(0);
	subresource.setBaseMipLevel(0);
	subresource.setLayerCount(1);
	subresource.setLevelCount(1);

	vk::ImageViewCreateInfo viewCreateInfo;
	viewCreateInfo.setViewType(vk::ImageViewType::e2D);
	viewCreateInfo.setComponents(vk::ComponentSwizzle{});
	viewCreateInfo.setFormat(swapChain.getSurfaceFormat().format);
	viewCreateInfo.setSubresourceRange(subresource);

	for (int i = 0; i < framesInFlight; i++)
	{
		ImageObject cur;

		vmaCreateImage(vmaAllocator, reinterpret_cast<VkImageCreateInfo *>(&imageCreateInfo), &allocCreateInfo,
					   reinterpret_cast<VkImage *>(&cur.image), &cur.imageAllocation, &cur.imageAllocationInfo);

		viewCreateInfo.setImage(cur.image);
		cur.imageView = device.createImageView(viewCreateInfo);

		renderTargets.push_back(cur);
	}
}

Model Renderer::createModel(const char *path)
{
	N::ModelCreateInfo createInfo{};
	createInfo.commandBuffer = commandBuffers[0];
	createInfo.descriptorPool = descriptorPool;
	createInfo.descriptorSetLayout = pipeline.getTextureSetLayout();
	createInfo.device = device;
	createInfo.queue = graphicsQueue;
	createInfo.sampler = sampler;
	createInfo.vmaAllocator = vmaAllocator;
	return Model(createInfo, path);
}
} // namespace N

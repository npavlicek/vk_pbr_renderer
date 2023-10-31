#include "VulkanManager.h"

VulkanManager::VulkanManager(GLFWwindow *window, int maxFramesInFlight)
{
	vk::raii::Context context;

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

	auto instance = util::createInstance(context, "Vulkan PBR Renderer", "Vulkan PBR Renderer", debugUtilsMessengerCreateInfo);

	vkState.debugMessenger = std::make_unique<vk::raii::DebugUtilsMessengerEXT>(std::move(instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfo)));

	auto physicalDevice = util::selectPhysicalDevice(instance);
	auto queueFamilyGraphicsIndex = util::selectQueueFamily(physicalDevice);
	auto device = util::createDevice(physicalDevice, queueFamilyGraphicsIndex);
	auto surface = util::createSurface(instance, window);
	auto swapChainFormat = util::selectSwapChainFormat(physicalDevice, surface);
	auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);

	vk::raii::SwapchainKHR swapChain{nullptr};
	vk::SwapchainCreateInfoKHR swapChainCreateInfo;
	std::tie(swapChain, swapChainCreateInfo) = util::createSwapChain(device, physicalDevice, surface, swapChainFormat, surfaceCapabilities);

	auto swapChainImages = swapChain.getImages();
	auto swapChainImageViews = util::createImageViews(device, swapChainImages, swapChainFormat.format, vk::ImageAspectFlagBits::eColor);
	auto commandPool = util::createCommandPool(device, queueFamilyGraphicsIndex);
	auto commandBuffers = util::createCommandBuffers(device, commandPool, maxFramesInFlight);
	auto shaderModules = util::createShaderModules(device, "shaders/vert.spv", "shaders/frag.spv");

	vkState.context = std::make_unique<vk::raii::Context>(std::move(context));
	vkState.physicalDevice = std::make_unique<vk::raii::PhysicalDevice>(std::move(physicalDevice));
	vkState.instance = std::make_unique<vk::raii::Instance>(std::move(instance));
	vkState.device = std::make_unique<vk::raii::Device>(std::move(device));
	vkState.surface = std::make_unique<vk::raii::SurfaceKHR>(std::move(surface));
	vkState.swapChain = std::make_unique<vk::raii::SwapchainKHR>(std::move(swapChain));
	vkState.swapChainImages = std::make_unique<std::vector<vk::Image>>(std::move(swapChainImages));
	vkState.swapChainImageViews = std::make_unique<std::vector<vk::raii::ImageView>>(std::move(swapChainImageViews));
	vkState.commandPool = std::make_unique<vk::raii::CommandPool>(std::move(commandPool));
	vkState.commandBuffers = std::make_unique<std::vector<vk::raii::CommandBuffer>>(std::move(commandBuffers));
	vkState.shaderModules = std::make_unique<std::vector<vk::raii::ShaderModule>>(std::move(shaderModules));

	vkState.swapChainCreateInfo = swapChainCreateInfo;
	vkState.queueFamilyGraphicsIndex = queueFamilyGraphicsIndex;
	vkState.swapChainFormat = swapChainFormat;
	vkState.surfaceCapabilities = surfaceCapabilities;
	vkState.maxFramesInFlight = maxFramesInFlight;
}

VulkanState &VulkanManager::getVulkanState()
{
	return vkState;
}

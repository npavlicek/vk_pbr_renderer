#include "Util.h"

namespace util {
	vk::raii::Instance createInstance(vk::raii::Context &context, const char *applicationName, const char *engineName) {
		std::vector<const char *> enabledLayers{};
		std::vector<const char *> enabledExtensions{};

		// Add GLFW necessary extensions
		uint32_t glfwExtensionCount = 0;
		const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		enabledExtensions.reserve(glfwExtensionCount);
		for (int i = 0; i < static_cast<int>(glfwExtensionCount); i++) {
			enabledExtensions.push_back(glfwExtensions[i]);
		}

		enabledExtensions.push_back("VK_KHR_get_surface_capabilities2");

		auto applicationInfo = vk::ApplicationInfo(applicationName,
		                                           1,
		                                           engineName,
		                                           1,
		                                           vk::ApiVersion13);


		auto instanceCreateInfo = vk::InstanceCreateInfo({},
		                                                 &applicationInfo,
		                                                 enabledLayers.size(),
		                                                 enabledLayers.data(),
		                                                 enabledExtensions.size(),
		                                                 enabledExtensions.data(),
		                                                 nullptr);

		return context.createInstance(instanceCreateInfo);
	}

	vk::raii::PhysicalDevice selectPhysicalDevice(vk::raii::Instance &instance) {
		auto physicalDevices = instance.enumeratePhysicalDevices();

		vk::raii::PhysicalDevice selectedDevice(nullptr);
		for (const auto &pDevice: physicalDevices) {
			if (pDevice.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
				selectedDevice = pDevice;
				break;
			}
		}

		std::cout << "Selected Device: " << selectedDevice.getProperties().deviceName << std::endl;

		return selectedDevice;
	}

	int selectQueueFamily(vk::raii::PhysicalDevice &physicalDevice) {
		auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

		int selectedQueueFamilyIndex = 0;
		for (const auto &queueFamily: queueFamilyProperties) {
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) break;
			selectedQueueFamilyIndex++;
		}

		return selectedQueueFamilyIndex;
	}

	vk::raii::Device createDevice(vk::raii::PhysicalDevice &physicalDevice) {
		int queueFamilyIndex = selectQueueFamily(physicalDevice);
		auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties()[queueFamilyIndex];

		std::vector<float> queuePriorities(queueFamilyProperties.queueCount,
		                                   1.f);

		vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
		deviceQueueCreateInfo.setQueueFamilyIndex(queueFamilyIndex);
		deviceQueueCreateInfo.setQueueCount(queueFamilyProperties.queueCount);
		deviceQueueCreateInfo.setQueuePriorities(queuePriorities);

		std::vector<const char *> enabledExtensions{"VK_KHR_swapchain"};

		vk::DeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.setQueueCreateInfoCount(1);
		deviceCreateInfo.setQueueCreateInfos(deviceQueueCreateInfo);
		deviceCreateInfo.setEnabledExtensionCount(enabledExtensions.size());
		deviceCreateInfo.setPEnabledExtensionNames(enabledExtensions);

		return {physicalDevice, deviceCreateInfo};
	}

	vk::raii::SurfaceKHR createSurface(vk::raii::Instance &instance, GLFWwindow *window) {
		VkSurfaceKHR surface = VK_NULL_HANDLE;

		glfwCreateWindowSurface(static_cast<VkInstance>(*instance),
		                        window,
		                        nullptr,
		                        &surface);

		return {instance, surface};
	}

	vk::raii::SwapchainKHR
	createSwapChain(vk::raii::Device &device, vk::raii::PhysicalDevice &physicalDevice, vk::raii::SurfaceKHR &surface) {
		auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
		auto presentModes = physicalDevice.getSurfacePresentModesKHR(*surface);
		auto formats = physicalDevice.getSurfaceFormatsKHR(*surface);

		vk::PresentModeKHR selectedPresentMode;
		for (const auto &presentMode: presentModes) {
			if (presentMode == vk::PresentModeKHR::eMailbox) {
				selectedPresentMode = presentMode;
				break;
			}
		}

		vk::Format selectedFormat;
		vk::ColorSpaceKHR selectedColorSpace;
		for (const auto &format: formats) {
			if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
				selectedFormat = format.format;
				selectedColorSpace = format.colorSpace;
				break;
			}
		}

		vk::SwapchainCreateInfoKHR swapChainCreateInfo;
		swapChainCreateInfo.setPresentMode(selectedPresentMode);
		swapChainCreateInfo.setImageFormat(selectedFormat);
		swapChainCreateInfo.setImageColorSpace(selectedColorSpace);
		swapChainCreateInfo.setSurface(*surface);

		swapChainCreateInfo.setImageArrayLayers(1);
		swapChainCreateInfo.setImageSharingMode(vk::SharingMode::eExclusive);
		swapChainCreateInfo.setImageUsage(vk::ImageUsageFlags{vk::ImageUsageFlagBits::eColorAttachment});
		swapChainCreateInfo.setClipped(vk::True);
		swapChainCreateInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
		swapChainCreateInfo.setMinImageCount(capabilities.minImageCount + 1);
		swapChainCreateInfo.setPreTransform(capabilities.currentTransform);

		swapChainCreateInfo.setImageExtent(capabilities.currentExtent);

		return {device, swapChainCreateInfo};
	}
} // pbr

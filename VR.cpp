#include "VR.h"

namespace pbr {
	VR::VR(GLFWwindow *window) {
		this->window = window;

		VkApplicationInfo vkApplicationInfo{};
		vkApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		vkApplicationInfo.pApplicationName = "Vulkan PBR Renderer";
		vkApplicationInfo.pEngineName = "Vulkan PBR Renderer";
		vkApplicationInfo.apiVersion = VK_API_VERSION_1_3;
		vkApplicationInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		vkApplicationInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);

		std::vector<const char *> enabledLayers{};
		std::vector<const char *> enabledExtensions{};

		// Add GLFW necessary extensions
		uint32_t glfwExtensionCount = 0;
		const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		enabledExtensions.reserve(glfwExtensionCount);
		for (int i = 0; i < glfwExtensionCount; i++) {
			enabledExtensions.push_back(glfwExtensions[i]);
		}

		VkInstanceCreateInfo vkInstanceCreateInfo{};
		vkInstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		vkInstanceCreateInfo.pApplicationInfo = &vkApplicationInfo;
		vkInstanceCreateInfo.enabledExtensionCount = enabledExtensions.size();
		vkInstanceCreateInfo.enabledLayerCount = enabledLayers.size();
		vkInstanceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
		vkInstanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

		VkResult result = vkCreateInstance(&vkInstanceCreateInfo,
				VK_NULL_HANDLE,
				&handles.instance);
		checkResult(result);

		// Create window surface
		result = glfwCreateWindowSurface(handles.instance, window, VK_NULL_HANDLE, &handles.surface);
		checkResult(result);

		setPhysicalDevice();

		QueueFamily selectedQueueFamily = getQueueFamily();

		std::vector<float> queuePriorities(selectedQueueFamily.queueCount);
		for (auto &cur: queuePriorities) {
			cur = 1.f;
		}

		VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
		deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueCreateInfo.queueCount = selectedQueueFamily.queueCount;
		deviceQueueCreateInfo.queueFamilyIndex = selectedQueueFamily.queueIndex;
		deviceQueueCreateInfo.pQueuePriorities = queuePriorities.data();

		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;

		std::vector<const char *> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();

		result = vkCreateDevice(handles.physicalDevice,
				&deviceCreateInfo,
				VK_NULL_HANDLE,
				&handles.device);
		checkResult(result);

		for (int queueIndex = 0; queueIndex < selectedQueueFamily.queueCount; queueIndex++) {
			VkQueue curQueue;
			vkGetDeviceQueue(handles.device, selectedQueueFamily.queueIndex, queueIndex, &curQueue);
			handles.queues.push_back(curQueue);
		}
	}

	QueueFamily VR::getQueueFamily() const {
		uint32_t queueFamilyPropertyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(handles.physicalDevice, &queueFamilyPropertyCount, VK_NULL_HANDLE);
		std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(handles.physicalDevice, &queueFamilyPropertyCount,
				queueFamilyProperties.data());

		int selectedQueueFamily = 0;
		uint32_t queueCount = 0;
		while (selectedQueueFamily != queueFamilyProperties.size()) {
			if (queueFamilyProperties[selectedQueueFamily].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				queueCount = queueFamilyProperties[selectedQueueFamily].queueCount;
				break;
			}
			selectedQueueFamily++;
		}

		return QueueFamily{selectedQueueFamily, queueCount};
	}

	void VR::setPhysicalDevice() {
		uint32_t deviceCount = 0;

		VkResult result = vkEnumeratePhysicalDevices(handles.instance,
				&deviceCount,
				nullptr);
		checkResult(result);

		std::vector<VkPhysicalDevice> devices(deviceCount);
		result = vkEnumeratePhysicalDevices(handles.instance,
				&deviceCount,
				devices.data());
		checkResult(result);

		for (const auto &device: devices) {
			VkPhysicalDeviceProperties properties;
			VkPhysicalDeviceFeatures features;
			vkGetPhysicalDeviceProperties(device,
					&properties);
			vkGetPhysicalDeviceFeatures(device,
					&features);

			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				handles.physicalDevice = device;
				break;
			}
		}
	}

	void VR::generateSwapchain() {
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(handles.physicalDevice, handles.surface, &capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(handles.physicalDevice, handles.surface, &formatCount, VK_NULL_HANDLE);
		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(handles.physicalDevice, handles.surface, &formatCount, formats.data());

		uint32_t presentModesCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(handles.physicalDevice, handles.surface, &presentModesCount,
				VK_NULL_HANDLE);
		std::vector<VkPresentModeKHR> presentModes(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(handles.physicalDevice, handles.surface, &presentModesCount,
				presentModes.data());

		VkSurfaceFormatKHR selectedFormat;
		for (const auto &cur: formats) {
			if (cur.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && cur.format == VK_FORMAT_B8G8R8A8_SRGB) {
				selectedFormat = cur;
				break;
			}
		}

		VkPresentModeKHR selectedPresentMode;
		for (const auto &cur: presentModes) {
			if (cur == VK_PRESENT_MODE_FIFO_KHR) {
				selectedPresentMode = cur;
				break;
			}
		}

		swapchainInfo.format = selectedFormat;
		swapchainInfo.presentMode = selectedPresentMode;

		VkSwapchainCreateInfoKHR swapchainCreateInfoKhr{};
		swapchainCreateInfoKhr.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfoKhr.minImageCount = capabilities.minImageCount + 1;
		swapchainCreateInfoKhr.clipped = VK_TRUE;
		swapchainCreateInfoKhr.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfoKhr.preTransform = capabilities.currentTransform;
		swapchainCreateInfoKhr.imageArrayLayers = 1;
		swapchainCreateInfoKhr.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCreateInfoKhr.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D extent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
		};

		extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent
				.height);

		swapchainCreateInfoKhr.imageExtent = extent;

		swapchainCreateInfoKhr.surface = handles.surface;
		swapchainCreateInfoKhr.presentMode = selectedPresentMode;
		swapchainCreateInfoKhr.imageFormat = selectedFormat.format;
		swapchainCreateInfoKhr.imageColorSpace = selectedFormat.colorSpace;

		VkResult result = vkCreateSwapchainKHR(handles.device, &swapchainCreateInfoKhr, VK_NULL_HANDLE, &handles
				.swapchain);
		checkResult(result);

		setSwapchainImages();
	}

	void VR::setSwapchainImages() {
		uint32_t numberImages;
		vkGetSwapchainImagesKHR(handles.device, handles.swapchain, &numberImages, VK_NULL_HANDLE);
		handles.swapchainImages.resize(numberImages);
		vkGetSwapchainImagesKHR(handles.device, handles.swapchain, &numberImages, handles.swapchainImages.data());
	}

	VulkanHandles VR::getHandles() {
		return handles;
	}

	void VR::checkResult(VkResult result) {
		if (result != VK_SUCCESS) {
			throw std::runtime_error(string_VkResult(result));
		}
	}

	VR::~VR() {
		vkDestroySwapchainKHR(handles.device, handles.swapchain, VK_NULL_HANDLE);
		vkDestroySurfaceKHR(handles.instance, handles.surface, VK_NULL_HANDLE);
		vkDestroyDevice(handles.device, VK_NULL_HANDLE);
		vkDestroyInstance(handles.instance, VK_NULL_HANDLE);
	}

}

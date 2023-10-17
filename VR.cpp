#include "VR.h"

namespace pbr {
	VR::VR() {
		VkApplicationInfo vkApplicationInfo{};
		vkApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		vkApplicationInfo.pApplicationName = "Vulkan PBR Renderer";
		vkApplicationInfo.pEngineName = "Vulkan PBR Renderer";
		vkApplicationInfo.apiVersion = VK_API_VERSION_1_3;
		vkApplicationInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		vkApplicationInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);

		const char *const enabledLayers = {};
		const char *const enabledExtensions = {};

		VkInstanceCreateInfo vkInstanceCreateInfo{};
		vkInstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		vkInstanceCreateInfo.pApplicationInfo = &vkApplicationInfo;
		vkInstanceCreateInfo.enabledExtensionCount = 0;
		vkInstanceCreateInfo.enabledLayerCount = 0;
		vkInstanceCreateInfo.ppEnabledLayerNames = &enabledLayers;
		vkInstanceCreateInfo.ppEnabledExtensionNames = &enabledExtensions;

		VkResult result = vkCreateInstance(&vkInstanceCreateInfo,
				VK_NULL_HANDLE,
				&handles.instance);
		checkResult(result);

		setPhysicalDevice();

		QueueFamily selectedQueueFamily = selectQueueFamily();

		VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
		deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueCreateInfo.queueCount = selectedQueueFamily.queueCount;
		deviceQueueCreateInfo.queueFamilyIndex = selectedQueueFamily.queueIndex;
		float queuePriority = 1.f;
		deviceQueueCreateInfo.pQueuePriorities = &queuePriority;

		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;

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

	QueueFamily VR::selectQueueFamily() const {
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

	void VR::checkResult(VkResult result) {
		if (result != VK_SUCCESS) {
			throw std::runtime_error(string_VkResult(result));
		}
	}

	VR::~VR() {
		vkDestroyDevice(handles.device,
				VK_NULL_HANDLE);
		vkDestroyInstance(handles.instance,
				VK_NULL_HANDLE);
	}
}

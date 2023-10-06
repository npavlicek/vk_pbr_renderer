#include "VkDeviceUtils.h"

namespace lvk {
	VkPhysicalDevice VkDeviceUtils::pickDevice(VkInstance instance) {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (!deviceCount)
			throw std::runtime_error("No GPUs detected which support Vulkan!");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		for (const auto &device: devices) {
			VkPhysicalDeviceFeatures physicalDeviceFeatures{};
			VkPhysicalDeviceProperties physicalDeviceProperties{};
			vkGetPhysicalDeviceFeatures(device, &physicalDeviceFeatures);
			vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);

			std::cout << physicalDeviceProperties.deviceType << std::endl;

			if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			    physicalDeviceFeatures.geometryShader) {
				return device;
			}
		}
	}

	queueFamilyIndices VkDeviceUtils::findQueueFamilyIndices(VkPhysicalDevice device) {
		queueFamilyIndices queueFamilyIndices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto &queueFamily: queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				queueFamilyIndices.graphicsFamily = i;

			if (queueFamilyIndices.isComplete())
				break;

			i++;
		}

		return queueFamilyIndices;
	}

	void VkDeviceUtils::createLogicalDevice(VkDevice *device, VkPhysicalDevice physicalDevice,
	                                        queueFamilyIndices queueFamilyIndices) {
		VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
		deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		deviceQueueCreateInfo.queueCount = 1;

		float queuePriority = 1.0;
		deviceQueueCreateInfo.pQueuePriorities = &queuePriority;

		VkPhysicalDeviceFeatures physicalDeviceFeatures{};
		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
		deviceCreateInfo.enabledExtensionCount = 0;

		if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, device) != VK_SUCCESS)
			throw std::runtime_error("Failed to create the logical device.");
	}
} // lvk

#include "vk_device_utils.h"

namespace lvk {
	VkPhysicalDevice vk_device_utils::pickDevice(VkInstance instance) {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (!deviceCount)
			throw std::runtime_error("No GPUs detected which support Vulkan!");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		for (const auto &device: devices) {
			VkPhysicalDeviceFeatures physicalDeviceFeatures;
			VkPhysicalDeviceProperties physicalDeviceProperties;
			vkGetPhysicalDeviceFeatures(device, &physicalDeviceFeatures);
			vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);

			if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			    physicalDeviceFeatures.geometryShader) {
				return device;
			}
		}
	}
} // lvk

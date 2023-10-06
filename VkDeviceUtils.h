#ifndef VK_PBR_RENDERER_VKDEVICEUTILS_H
#define VK_PBR_RENDERER_VKDEVICEUTILS_H

#include <vulkan/vulkan.hpp>
#include <optional>
#include <iostream>

namespace lvk {
	struct queueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;

		[[nodiscard]] bool isComplete() const {
			return graphicsFamily.has_value();
		}
	};

	class VkDeviceUtils {
	public:
		static VkPhysicalDevice pickDevice(VkInstance instance);
		static void
		createLogicalDevice(VkDevice *device, VkPhysicalDevice physicalDevice, queueFamilyIndices queueFamilyIndices);
		static queueFamilyIndices findQueueFamilyIndices(VkPhysicalDevice device);
	};
} // lvk

#endif //VK_PBR_RENDERER_VKDEVICEUTILS_H

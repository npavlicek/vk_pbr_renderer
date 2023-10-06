#ifndef VK_PBR_RENDERER_VK_DEVICE_UTILS_H
#define VK_PBR_RENDERER_VK_DEVICE_UTILS_H

#include <vulkan/vulkan.hpp>

namespace lvk {
	class vk_device_utils {
	public:
		static VkPhysicalDevice pickDevice(VkInstance instance);
	};
} // lvk

#endif //VK_PBR_RENDERER_VK_DEVICE_UTILS_H

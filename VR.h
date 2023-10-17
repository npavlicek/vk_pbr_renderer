#ifndef VK_PBR_RENDERER_VR_H
#define VK_PBR_RENDERER_VR_H

#include <vulkan/vulkan.hpp>
#include <vulkan/vk_enum_string_helper.h>
#include <exception>
#include <iostream>

namespace pbr {
	struct VulkanHandles {
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		std::vector<VkQueue> queues;
	};

	struct QueueFamily {
		int queueIndex;
		uint32_t queueCount;
	};

	class VR {
	public:
		VR();
		~VR();

	private:
		VulkanHandles handles{};

		QueueFamily selectQueueFamily() const;
		void setPhysicalDevice();
		static void checkResult(VkResult result);
	};
}

#endif //VK_PBR_RENDERER_VR_H

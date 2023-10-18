#ifndef VK_PBR_RENDERER_VR_H
#define VK_PBR_RENDERER_VR_H

#include <vulkan/vulkan.hpp>
#include <vulkan/vk_enum_string_helper.h>
#include <GLFW/glfw3.h>
#include <exception>
#include <iostream>

namespace pbr {
	struct VulkanHandles {
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		std::vector<VkQueue> queues;
		VkSurfaceKHR surface;
		VkSwapchainKHR swapchain;
		std::vector<VkImage> swapchainImages;
	};

	struct SwapchainInfo {
		VkSurfaceFormatKHR format;
		VkPresentModeKHR presentMode;
	};

	struct QueueFamily {
		int queueIndex;
		uint32_t queueCount;
	};

	class VR {
	public:
		VR(GLFWwindow *window);
		~VR();
		VulkanHandles getHandles();
		void generateSwapchain();

	private:
		VulkanHandles handles{};
		SwapchainInfo swapchainInfo{};
		GLFWwindow *window;

		void setSwapchainImages();
		QueueFamily getQueueFamily() const;
		void setPhysicalDevice();
		static void checkResult(VkResult result);
	};
}

#endif //VK_PBR_RENDERER_VR_H

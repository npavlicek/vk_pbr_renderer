#ifndef VK_PBR_RENDERER_APP_H
#define VK_PBR_RENDERER_APP_H

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <cstdlib>
#include <iostream>

#include "vk_init_utils.h"

namespace lvk {
	class app {
	public:
		void init();

		void loop();
		void cleanup();

	private:
		GLFWwindow *window = nullptr;
		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;

		void initVulkan();
		void createInstance();
		void setupDebugMessenger();

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
				VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverityFlagBitsExt,
				VkDebugUtilsMessageTypeFlagsEXT messageTypeFlagsExt,
				const VkDebugUtilsMessengerCallbackDataEXT *callbackDataExt,
				void *pUserData
		) {
			std::cout << "Validation layer: " << callbackDataExt->pMessage << std::endl;
			return VK_FALSE;
		}
	};
} // lvk

#endif //VK_PBR_RENDERER_APP_H

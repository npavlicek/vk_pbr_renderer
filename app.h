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
		VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoExt{};

		void initVulkan();
		void createInstance();
		void setupDebugMessenger();
		void createDebugMessenger();

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
				VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverityFlagBitsExt,
				VkDebugUtilsMessageTypeFlagsEXT messageTypeFlagsExt,
				const VkDebugUtilsMessengerCallbackDataEXT *callbackDataExt,
				void *pUserData
		) {
			switch (messageTypeFlagsExt) {
				case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
					std::cout << "[VkDebugUtils-General]: ";
					break;
				case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
					std::cout << "[VkDebugUtils-Validation]: ";
					break;
				case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
					std::cout << "[VkDebugUtils-Performance]: ";
					break;
				default:
					std::cout << "[VkDebugUtils-Unknown]: ";
			}
			switch (messageSeverityFlagBitsExt) {
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
					std::cout << "INFO - ";
					break;
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
					std::cout << "VERBOSE - ";
					break;
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
					std::cout << "ERROR - ";
					break;
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
					std::cout << "WARNING - ";
					break;
				default:
					std::cout << "UNKNOWN SEVERITY - ";
			}
			std::cout << callbackDataExt->pMessage << std::endl;
			return VK_FALSE;
		}
	};
} // lvk

#endif //VK_PBR_RENDERER_APP_H

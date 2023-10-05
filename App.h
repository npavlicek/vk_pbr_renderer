#ifndef VK_PBR_RENDERER_APP_H
#define VK_PBR_RENDERER_APP_H

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

#include "vk_init_utils.h"

//#define VK_DEBUG_FILE_OUTPUT
#define ENABLE_ANSI_COLORS

#include "ansi_color_defs.h"

namespace lvk {
	class App {
	public:
		void init();

		void loop();
		void cleanup();

	private:
		GLFWwindow *window = nullptr;
		VkInstance instance{};
		VkDebugUtilsMessengerEXT debugMessenger{};
		VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoExt{};

#ifdef VK_DEBUG_FILE_OUTPUT
		static const auto inline file_stream = std::ofstream("log");
#endif

		void initVulkan();
		void createInstance();
		void setupDebugMessenger();
		void createDebugMessenger();

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
				VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverityFlagBitsExt,
				VkDebugUtilsMessageTypeFlagsEXT messageTypeFlagsExt,
				const VkDebugUtilsMessengerCallbackDataEXT *callbackDataExt,
				void *pUserData
		);
	};
} // lvk

#endif //VK_PBR_RENDERER_APP_H

#ifndef VK_PBR_RENDERER_APP_H
#define VK_PBR_RENDERER_APP_H

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <cstdlib>
#include <iostream>

#include "vlayers_util.h"

namespace lvk {
	class app {
	public:
		void init();

		void loop();
		void cleanup();

	private:
		GLFWwindow *window = nullptr;
		VkInstance instance;

		void initVulkan();
		void createInstance();
	};
} // lvk

#endif //VK_PBR_RENDERER_APP_H

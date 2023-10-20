#include "Util.h"

#include <vulkan/vulkan_to_string.hpp>

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window,
		                         GLFW_TRUE);
	}
}

int main() {
	if (!glfwInit())
		throw std::runtime_error("Could not initialize GLFW");

	glfwWindowHint(GLFW_RESIZABLE,
	               GLFW_FALSE);
	glfwWindowHint(GLFW_CLIENT_API,
	               GLFW_NO_API);

	GLFWwindow *window = glfwCreateWindow(1280,
	                                      720,
	                                      "Testing Vulkan!",
	                                      nullptr,
	                                      nullptr);

	glfwSetKeyCallback(window,
	                   keyCallback);

	vk::raii::Context context;

	auto instance = util::createInstance(context,
	                                     "Vulkan PBR Renderer",
	                                     "Vulkan PBR Renderer");
	auto physicalDevice = util::selectPhysicalDevice(instance);
	auto device = util::createDevice(physicalDevice);
	auto surface = util::createSurface(instance,
	                                   window);
	auto swapChain = util::createSwapChain(device,
	                                       physicalDevice,
	                                       surface);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

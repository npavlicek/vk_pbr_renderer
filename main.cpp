#include "VR.h"

#define log_error(type, what) std::cerr << "Encountered a " << type << " error: " << what << std::endl;

int main() {
	if (!glfwInit())
		throw std::runtime_error("Could not initialize GLFW");

	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow *window = glfwCreateWindow(1280, 720, "Testing Vulkan!", nullptr, nullptr);

	pbr::VR vulkanRenderer(window);
	vulkanRenderer.generateSwapchain();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

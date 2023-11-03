#define GLM_ENABLE_EXPERIMENTAL

#include "Renderer.h"
#include "Mesh.h"
#include "Model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void keyCallback(
	GLFWwindow *window,
	int key,
	int scancode,
	int action,
	int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(
			window,
			GLFW_TRUE);
	}
}

int main()
{
	if (!glfwInit())
		throw std::runtime_error("Could not initialize GLFW");

	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow *window = glfwCreateWindow(
		1600,
		900,
		"Testing Vulkan!",
		nullptr,
		nullptr);

	if (!window)
	{
		std::cerr << "Could not open primary window!" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(
		window,
		keyCallback);

	Renderer renderer(window);

	auto objLoadStartTime = std::chrono::high_resolution_clock::now();

	Model model = renderer.createModel("models/cow.obj");

	auto objLoadEndTime = std::chrono::high_resolution_clock::now();

	auto elapsedTime = static_cast<std::chrono::duration<float, std::chrono::milliseconds::period>>(objLoadEndTime -
																									objLoadStartTime);

	std::cout << "Took " << elapsedTime.count() << " milliseconds to load OBJ model" << std::endl;

	Texture texture = renderer.createTexture("res/monkey.png");

	renderer.updateDescriptorSets(texture);

	renderer.loop(model);

	renderer.destroy();

	return 0;
}

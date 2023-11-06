#define GLM_ENABLE_EXPERIMENTAL

#include "Mesh.h"
#include "Model.h"
#include "Renderer.h"
#include "Texture.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

glm::vec3 cameraPos = glm::vec3{0.f, 0.f, -5.f};
glm::vec3 cameraFront = glm::vec3(0.f, 0.f, 1.f);
glm::vec3 cameraUp = glm::vec3(0.f, -1.f, 0.f);
float cameraSpeed = 10.f;

struct
{
	bool forward;
	bool backward;
	bool left;
	bool right;
} m;

bool cursorEnabled = false;

GLFWwindow *window = nullptr;

float lastX = 1600 / 2, lastY = 900 / 2;

float yaw, pitch;

void calculateCameraDir()
{
	double cursorX, cursorY;
	glfwGetCursorPos(window, &cursorX, &cursorY);

	std::cout << cursorX << std::endl;

	float xOffset = lastX - cursorX;
	float yOffset = cursorY - lastY;
	lastX = cursorX;
	lastY = cursorY;

	std::cout << cursorX << " " << xOffset << " " << lastX << std::endl;

	const float sens = 0.1f;
	xOffset *= sens;
	yOffset *= sens;

	yaw += xOffset;
	pitch += yOffset;

	if (pitch > 89.f)
		pitch = 89.f;
	if (pitch < -89.f)
		pitch = -89.f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(direction);
}

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

	if (key == GLFW_KEY_W && action == GLFW_PRESS)
	{
		m.forward = true;
	}
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
	{
		m.backward = true;
	}
	if (key == GLFW_KEY_D && action == GLFW_PRESS)
	{
		m.right = true;
	}
	if (key == GLFW_KEY_A && action == GLFW_PRESS)
	{
		m.left = true;
	}

	if (key == GLFW_KEY_E && action == GLFW_PRESS)
	{
		if (cursorEnabled)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			cursorEnabled = false;
		}
		else
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			cursorEnabled = true;
		}
	}

	if (key == GLFW_KEY_W && action == GLFW_RELEASE)
	{
		m.forward = false;
	}
	if (key == GLFW_KEY_S && action == GLFW_RELEASE)
	{
		m.backward = false;
	}
	if (key == GLFW_KEY_D && action == GLFW_RELEASE)
	{
		m.right = false;
	}
	if (key == GLFW_KEY_A && action == GLFW_RELEASE)
	{
		m.left = false;
	}
}

int main()
{
	if (!glfwInit())
		throw std::runtime_error("Could not initialize GLFW");

	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(
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

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	Renderer renderer(window);

	auto objLoadStartTime = std::chrono::high_resolution_clock::now();

	Model model = renderer.createModel("models/cow.obj");
	Model model2 = renderer.createModel("models/teddybear.obj");

	auto objLoadEndTime = std::chrono::high_resolution_clock::now();

	auto elapsedTime = static_cast<std::chrono::duration<float, std::chrono::milliseconds::period>>(objLoadEndTime -
																									objLoadStartTime);

	std::cout << "Took " << elapsedTime.count() << " milliseconds to load OBJ model" << std::endl;

	Texture texture = renderer.createTexture("res/monkey.png");

	renderer.updateDescriptorSets(texture);

	glm::mat4 model2Pos = glm::translate(glm::vec3(15.f, 0.f, 0.f));
	model2Pos = glm::scale(model2Pos, glm::vec3(0.25f));
	model2.setModel(model2Pos);

	std::vector<Model> models;
	models.push_back(std::move(model));
	models.push_back(std::move(model2));

	renderer.resetCommandBuffers();

	auto lastTime = std::chrono::high_resolution_clock::now();

	while (!glfwWindowShouldClose(window))
	{
		auto currentTime = std::chrono::high_resolution_clock::now();
		float delta = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
		lastTime = currentTime;

		glfwPollEvents();

		if (!cursorEnabled)
		{
			calculateCameraDir();
		}

		if (m.forward)
		{
			cameraPos += cameraSpeed * cameraFront * delta;
		}
		if (m.backward)
		{
			cameraPos -= cameraSpeed * cameraFront * delta;
		}
		if (m.right)
		{
			cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed * delta;
		}
		if (m.left)
		{
			cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed * delta;
		}

		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

		renderer.render(models, view);
	}

	renderer.destroyModel(models.at(0));
	renderer.destroyModel(models.at(1));

	renderer.destroy();

	return 0;
}

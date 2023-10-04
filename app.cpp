#include "app.h"

namespace lvk {
	void app::init() {
		if (!glfwInit())
			throw std::runtime_error("Could not initialize GLFW.");

		initVulkan();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		window = glfwCreateWindow(1280, 720, "Vulkan window", nullptr, nullptr);
		if (!window)
			throw std::runtime_error("Could not create the GLFW window.");
	}

	void app::initVulkan() {
		createInstance();
	}

	void app::createInstance() {
#ifdef VLAYERS_ENABLED
		if (!vlayers_util::checkValidationLayerSupport())
			throw std::runtime_error("Validation layers were requested but are not available.");

		std::cout << "Validation layers enabled" << std::endl;
#endif

		VkApplicationInfo applicationInfo{};
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pApplicationName = "VK_PBR_RENDERER";
		applicationInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		applicationInfo.pEngineName = "idk";
		applicationInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		applicationInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &applicationInfo;

		const char **glfwExtensions;
		uint32_t glfwExtensionCount = 0;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;
		createInfo.enabledLayerCount = 0;

#ifdef VLAYERS_ENABLED
		createInfo.enabledLayerCount = static_cast<uint32_t>(vlayers_util::validationLayers.size());
		createInfo.ppEnabledLayerNames = vlayers_util::validationLayers.data();
#endif

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
			throw std::runtime_error("Failed to create the Vulkan instance!");

		std::cout << "Created Vulkan instance, API version - " << applicationInfo.apiVersion << std::endl;
	}

	void app::loop() {
		while (!glfwWindowShouldClose(window))
			glfwPollEvents();
	}

	void app::cleanup() {
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}
} // lvk

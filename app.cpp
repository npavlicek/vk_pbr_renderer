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
#ifdef VLAYERS_ENABLED
		setupDebugMessenger();
#endif

		createInstance();

#ifdef VLAYERS_ENABLED
		createDebugMessenger();
#endif
	}

	void app::setupDebugMessenger() {
		debugUtilsMessengerCreateInfoExt.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugUtilsMessengerCreateInfoExt.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugUtilsMessengerCreateInfoExt.messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugUtilsMessengerCreateInfoExt.pfnUserCallback = debugCallback;
	}

	void app::createDebugMessenger() {
		if (vk_init_utils::createDebugUtilsMessengerExt(instance, &debugUtilsMessengerCreateInfoExt, nullptr,
		                                                &debugMessenger) !=
		    VK_SUCCESS)
			throw std::runtime_error("Could not create VkDebugUtilsMessengerEXT");
	}

	void app::createInstance() {
#ifdef VLAYERS_ENABLED
		if (!vk_init_utils::checkValidationLayerSupport())
			throw std::runtime_error("Validation layers were requested but are not available.");

		std::cout << "Validation layers enabled" << std::endl;
#else
		std::cout << "Validation layers not enabled" << std::endl;
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

		auto extensions = vk_init_utils::getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		createInfo.enabledLayerCount = 0;

#ifdef VLAYERS_ENABLED
		createInfo.enabledLayerCount = static_cast<uint32_t>(vk_init_utils::validationLayers.size());
		createInfo.ppEnabledLayerNames = vk_init_utils::validationLayers.data();
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugUtilsMessengerCreateInfoExt;
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
#ifdef VLAYERS_ENABLED
		vk_init_utils::destroyDebugUtilsMessengerExt(instance, debugMessenger, nullptr);
#endif
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}
} // lvk

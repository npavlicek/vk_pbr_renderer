#include "App.h"

namespace lvk {
	void App::init() {
		if (!glfwInit())
			throw std::runtime_error("Could not initialize GLFW.");

		initVulkan();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		window = glfwCreateWindow(1280, 720, "Vulkan window", nullptr, nullptr);
		if (!window)
			throw std::runtime_error("Could not create the GLFW window.");
	}

	void App::initVulkan() {
#ifdef VLAYERS_ENABLED
		setupDebugMessenger();
#endif

		createInstance();

#ifdef VLAYERS_ENABLED
		createDebugMessenger();
#endif

		physicalDevice = vk_device_utils::pickDevice(instance);
	}

	void App::setupDebugMessenger() {
		debugUtilsMessengerCreateInfoExt.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugUtilsMessengerCreateInfoExt.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugUtilsMessengerCreateInfoExt.messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugUtilsMessengerCreateInfoExt.pfnUserCallback = debugCallback;
		debugUtilsMessengerCreateInfoExt.pNext = nullptr;
		debugUtilsMessengerCreateInfoExt.pUserData = nullptr;
		debugUtilsMessengerCreateInfoExt.flags = 0;
	}

	void App::createDebugMessenger() {
		if (vk_init_utils::createDebugUtilsMessengerExt(instance, &debugUtilsMessengerCreateInfoExt, nullptr,
		                                                &debugMessenger) !=
		    VK_SUCCESS)
			throw std::runtime_error("Could not create VkDebugUtilsMessengerEXT");
	}

	void App::createInstance() {
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

	void App::loop() {
		while (!glfwWindowShouldClose(window))
			glfwPollEvents();
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL App::debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverityFlagBitsExt,
			VkDebugUtilsMessageTypeFlagsEXT messageTypeFlagsExt,
			const VkDebugUtilsMessengerCallbackDataEXT *callbackDataExt,
			void *pUserData
	) {
#ifdef VK_DEBUG_FILE_OUTPUT
		auto old_stream = std::cout.rdbuf();
		std::cout.rdbuf(file_stream.rdbuf());
#endif
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
				ANSI_COLOR_WHITE
				std::cout << "INFO - ";
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				ANSI_COLOR_CYAN
				std::cout << "VERBOSE - ";
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				ANSI_COLOR_RED
				std::cout << "ERROR - ";
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				ANSI_COLOR_YELLOW
				std::cout << "WARNING - ";
				break;
			default:
				ANSI_COLOR_MAGENTA
				std::cout << "UNKNOWN SEVERITY - ";
		}
		std::cout << callbackDataExt->pMessage << std::endl;
		ANSI_COLOR_RESET

#ifdef VK_DEBUG_FILE_OUTPUT
		std::cout.rdbuf(old_stream);
#endif

		return VK_FALSE;
	}

	void App::cleanup() {
#ifdef VLAYERS_ENABLED
		vk_init_utils::destroyDebugUtilsMessengerExt(instance, debugMessenger, nullptr);
#endif

		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}
} // lvk

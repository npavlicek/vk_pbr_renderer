#include "vk_init_utils.h"

namespace lvk {
	bool vk_init_utils::checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char *layerName: validationLayers) {
			bool layerFound = false;

			for (const auto &layerProperties: availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
				return false;
		}

		return true;
	}

	std::vector<const char *> vk_init_utils::getRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef VLAYERS_ENABLED
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

		return extensions;
	}

	VkResult vk_init_utils::createDebugUtilsMessengerExt(VkInstance instance,
	                                                     const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
	                                                     const VkAllocationCallbacks *pAllocator,
	                                                     VkDebugUtilsMessengerEXT *pDebugMessenger) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
		                                                                       "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		else
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

} // lvk

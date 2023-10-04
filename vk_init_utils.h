#ifndef VK_PBR_RENDERER_VK_INIT_UTILS_H
#define VK_PBR_RENDERER_VK_INIT_UTILS_H

#ifndef NDEBUG
#define VLAYERS_ENABLED
#endif

#include <vector>
#include <stdexcept>
#include <cstdint>
#include <cstring>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace lvk {
	/**
	 * Utilities class for initializing validation layers
	 */
	class vk_init_utils {
	public:
		const static inline std::vector<const char *> validationLayers{"VK_LAYER_KHRONOS_validation"};

		static bool checkValidationLayerSupport();
		static std::vector<const char *> getRequiredExtensions();
		static VkResult createDebugUtilsMessengerExt(VkInstance instance,
		                                             const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
		                                             const VkAllocationCallbacks *pAllocator,
		                                             VkDebugUtilsMessengerEXT *pDebugMessenger);
	};
} // lvk

#endif //VK_PBR_RENDERER_VK_INIT_UTILS_H

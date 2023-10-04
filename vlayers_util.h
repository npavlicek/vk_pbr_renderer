#ifndef VK_PBR_RENDERER_VLAYERS_UTIL_H
#define VK_PBR_RENDERER_VLAYERS_UTIL_H

#ifndef NDEBUG
#define VLAYERS_ENABLED
#endif

#include <vector>
#include <stdexcept>
#include <cstdint>
#include <cstring>

#include <vulkan/vulkan.hpp>

namespace lvk {
	/**
	 * Utilities class for initializing validation layers
	 */
	class vlayers_util {
	public:
		const static inline std::vector<const char *> validationLayers{"VK_LAYER_KHRONOS_validation"};

		static bool checkValidationLayerSupport();
	};
} // lvk

#endif //VK_PBR_RENDERER_VLAYERS_UTIL_H

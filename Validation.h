#ifndef VK_PBR_RENDERER_VALIDATION_H
#define VK_PBR_RENDERER_VALIDATION_H

#include "vulkan/vulkan_raii.hpp"

#include "string"
#include "iostream"

class Validation {
public:
	static void getValidationLayers(
			vk::raii::Context &context,
			std::vector<const char *> &enabledLayers
	) {
		std::vector<std::string> requestedLayers{
				"VK_LAYER_KHRONOS_validation",
				"VK_LAYER_LUNARG_monitor"
		};

		auto availableLayers = context.enumerateInstanceLayerProperties();

		for (const auto &currentLayer: requestedLayers) {
			bool isLayerAvailable = std::any_of(
					availableLayers.begin(),
					availableLayers.end(),
					[currentLayer](vk::LayerProperties layer) -> bool {
						if (currentLayer == std::string(layer.layerName)) return true;
						return false;
					}
			);
			if (isLayerAvailable) {
				enabledLayers.push_back(
						currentLayer.c_str()
				);
			} else {
				std::cerr << "Requested vulkan layer: " << currentLayer << " is not available!" << std::endl;
			}
		}
	}
};

#endif //VK_PBR_RENDERER_VALIDATION_H

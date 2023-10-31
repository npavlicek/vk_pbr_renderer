#ifndef VK_PBR_RENDERER_VALIDATION_H
#define VK_PBR_RENDERER_VALIDATION_H

#include "vulkan/vulkan_raii.hpp"

#include "string"
#include "iostream"

class Validation
{
public:
	static void areLayersAvailable(vk::raii::Context &context, std::vector<const char *> &requestedLayers)
	{
		auto availableLayers = context.enumerateInstanceLayerProperties();

		bool available = true;
		for (const auto &layer : requestedLayers)
		{
			available = std::any_of(availableLayers.begin(), availableLayers.end(),
									[layer](vk::LayerProperties &props) -> bool
									{
										if (strcmp(layer, props.layerName) == 0)
											return true;
										return false;
									});
			if (!available)
				break;
		}

		if (!available)
		{
			throw std::runtime_error("Requested validation layers unavailable!");
		}
	}
};

#endif // VK_PBR_RENDERER_VALIDATION_H

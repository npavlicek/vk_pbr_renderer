#pragma once

#include <algorithm>
#include <vulkan/vulkan.hpp>

class Validation
{
  public:
	static void areLayersAvailable(std::vector<const char *> &requestedLayers)
	{
		auto availableLayers = vk::enumerateInstanceLayerProperties();

		bool available = true;
		for (const auto &layer : requestedLayers)
		{
			available =
				std::any_of(availableLayers.begin(), availableLayers.end(), [&layer](const vk::LayerProperties &cur) {
					if (strcmp(layer, cur.layerName) == 0)
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

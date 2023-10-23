#ifndef VK_PBR_RENDERER_VKERRORHANDLING_H
#define VK_PBR_RENDERER_VKERRORHANDLING_H

#include <iostream>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_to_string.hpp>

class VkResCheck {
public:
	VkResCheck() {};

	VkResCheck(const vk::Result &rhs) { // NOLINT
		lastResult = rhs;
		checkResult(rhs);
	}

	VkResCheck(const VkResult &rhs) {
		lastResult = vk::Result(rhs);
		checkResult(lastResult);
	}

	static void checkResult(
			vk::Result result
	) {
		if (result != vk::Result::eSuccess) {
			std::cerr << "Encountered a Vulkan error:\n";
			std::cerr << vk::to_string(result) << std::endl;
			throw std::runtime_error("Vulkan runtime exception");
		}
	}

private:
	vk::Result lastResult;
};

#endif //VK_PBR_RENDERER_VKERRORHANDLING_H

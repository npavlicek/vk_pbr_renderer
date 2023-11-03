#pragma once

#include "vulkan/vulkan_raii.hpp"

class CommandBuffer
{
public:
	static void beginSTC(const vk::CommandBuffer &buffer);
	static void endSTC(const vk::CommandBuffer &buffer, const vk::Queue &queue);
};

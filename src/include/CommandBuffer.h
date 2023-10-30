#ifndef VK_PBR_RENDERER_COMMANDBUFFER_H
#define VK_PBR_RENDERER_COMMANDBUFFER_H

#include "vulkan/vulkan_raii.hpp"

class CommandBuffer {
public:
	static void beginSTC(vk::raii::CommandBuffer &buffer);
	static void endSTC(
			vk::raii::CommandBuffer &buffer,
			vk::raii::Queue &queue
	);
};

#endif //VK_PBR_RENDERER_COMMANDBUFFER_H

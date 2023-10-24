#ifndef VK_PBR_RENDERER_FRAME_H
#define VK_PBR_RENDERER_FRAME_H

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace pbr {
	class Frame {
	public:
		static void begin(
				vk::raii::CommandBuffer &commandBuffer,
				vk::raii::Pipeline &pipeline,
				vk::raii::Buffer &vertexBuffer,
				vk::raii::Buffer &indexBuffer
		);
		static void end(vk::raii::CommandBuffer &commandBuffer);
		static void beginRenderPass(
				vk::raii::CommandBuffer &commandBuffer,
				vk::raii::RenderPass &renderPass,
				vk::raii::Framebuffer &frameBuffer,
				vk::ClearColorValue clearColorValue,
				vk::Rect2D renderArea,
				vk::SubpassContents subPassContents = vk::SubpassContents::eInline
		);
		static void endRenderPass(vk::raii::CommandBuffer &commandBuffer);
		static void draw(
				vk::raii::CommandBuffer &commandBuffer,
				vk::Rect2D renderArea,
				int vertexCount
		);
	};
} // pbr

#endif //VK_PBR_RENDERER_FRAME_H

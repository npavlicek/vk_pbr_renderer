#include "Frame.h"

namespace pbr {
	void Frame::begin(
			vk::raii::CommandBuffer &commandBuffer,
			vk::raii::Pipeline &pipeline
	) {
		commandBuffer.reset();
		commandBuffer.begin({});
		commandBuffer.bindPipeline(
				vk::PipelineBindPoint::eGraphics,
				*pipeline
		);
	}

	void Frame::end(vk::raii::CommandBuffer &commandBuffer) {
		commandBuffer.end();
	}

	void Frame::beginRenderPass(
			vk::raii::CommandBuffer &commandBuffer,
			vk::raii::RenderPass &renderPass,
			vk::raii::Framebuffer &frameBuffer,
			vk::ClearColorValue clearColorValue,
			vk::Rect2D renderArea,
			vk::SubpassContents subPassContents
	) {
		vk::RenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.setRenderArea(renderArea);
		renderPassBeginInfo.setRenderPass(*renderPass);
		renderPassBeginInfo.setFramebuffer(*frameBuffer);

		vk::ClearValue clearValue{clearColorValue};

		renderPassBeginInfo.setClearValues(clearValue);
		renderPassBeginInfo.setClearValueCount(1);

		commandBuffer.beginRenderPass(
				renderPassBeginInfo,
				subPassContents
		);
	}

	void Frame::endRenderPass(vk::raii::CommandBuffer &commandBuffer) {
		commandBuffer.endRenderPass();
	}

	void Frame::draw(
			vk::raii::CommandBuffer &commandBuffer,
			vk::Rect2D renderArea
	) {
		vk::Viewport viewport{
				static_cast<float>(renderArea.offset.x),
				static_cast<float>(renderArea.offset.y),
				static_cast<float>(renderArea.extent.width),
				static_cast<float>(renderArea.extent.height),
				0,
				1
		};
		commandBuffer.setScissor(
				0,
				renderArea
		);
		commandBuffer.setViewport(
				0,
				viewport
		);
		commandBuffer.draw(
				3,
				1,
				0,
				0
		);
	}
} // pbr

#include "Frame.h"

namespace pbr {
	void Frame::begin(
			vk::raii::CommandBuffer &commandBuffer,
			vk::raii::Pipeline &pipeline,
			vk::raii::PipelineLayout &pipelineLayout,
			vk::raii::Buffer &vertexBuffer,
			vk::raii::Buffer &indexBuffer,
			vk::raii::DescriptorSet &descriptorSet
	) {
		commandBuffer.reset();
		commandBuffer.begin({});

		commandBuffer.bindPipeline(
				vk::PipelineBindPoint::eGraphics,
				*pipeline
		);

		commandBuffer.bindVertexBuffers(
				0,
				*vertexBuffer,
				vk::DeviceSize{0}
		);

		commandBuffer.bindIndexBuffer(
				*indexBuffer,
				0,
				vk::IndexType::eUint16
		);

		commandBuffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				*pipelineLayout,
				0,
				*descriptorSet,
				nullptr
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
			vk::Rect2D renderArea,
			int vertexCount
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
		commandBuffer.drawIndexed(
				vertexCount,
				1,
				0,
				0,
				0
		);
	}
} // pbr

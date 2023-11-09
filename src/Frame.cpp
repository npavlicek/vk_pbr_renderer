#include "Frame.h"

#include <iostream>

namespace pbr
{
void Frame::begin(vk::raii::CommandBuffer &commandBuffer, vk::raii::Pipeline &pipeline,
				  vk::raii::PipelineLayout &pipelineLayout, const vk::DescriptorSet &descriptorSet,
				  vk::Buffer &vertexBuffer, vk::Buffer &indexBuffer)
{
	commandBuffer.reset();
	commandBuffer.begin({});

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
}

void Frame::end(vk::raii::CommandBuffer &commandBuffer)
{
	commandBuffer.end();
}

void Frame::beginRenderPass(vk::raii::CommandBuffer &commandBuffer, vk::raii::RenderPass &renderPass,
							vk::raii::Framebuffer &frameBuffer, std::vector<vk::ClearValue> &clearValues,
							vk::Rect2D renderArea, vk::SubpassContents subPassContents)
{
	vk::RenderPassBeginInfo renderPassBeginInfo;
	renderPassBeginInfo.setRenderArea(renderArea);
	renderPassBeginInfo.setRenderPass(*renderPass);
	renderPassBeginInfo.setFramebuffer(*frameBuffer);

	renderPassBeginInfo.setClearValues(*clearValues.data());
	renderPassBeginInfo.setClearValueCount(clearValues.size());

	commandBuffer.beginRenderPass(renderPassBeginInfo, subPassContents);
}

void Frame::endRenderPass(vk::raii::CommandBuffer &commandBuffer)
{
	commandBuffer.endRenderPass();
}

void Frame::draw(vk::raii::CommandBuffer &commandBuffer, vk::Rect2D renderArea, int vertexCount)
{
	vk::Viewport viewport{static_cast<float>(renderArea.offset.x),
						  static_cast<float>(renderArea.extent.height),
						  static_cast<float>(renderArea.extent.width),
						  -static_cast<float>(renderArea.extent.height),
						  0,
						  1};

	commandBuffer.setScissor(0, renderArea);
	commandBuffer.setViewport(0, viewport);
	commandBuffer.drawIndexed(vertexCount, 1, 0, 0, 0);
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *commandBuffer);
}
} // namespace pbr

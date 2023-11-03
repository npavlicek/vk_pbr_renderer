#include "CommandBuffer.h"

void CommandBuffer::beginSTC(const vk::CommandBuffer &buffer)
{
	buffer.reset();

	vk::CommandBufferBeginInfo commandBufferBeginInfo;
	commandBufferBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	buffer.begin(commandBufferBeginInfo);
}

void CommandBuffer::endSTC(const vk::CommandBuffer &buffer, const vk::Queue &queue)
{
	buffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.setCommandBufferCount(1);
	submitInfo.setCommandBuffers(buffer);

	queue.submit(submitInfo);
	queue.waitIdle();

	buffer.reset();
}

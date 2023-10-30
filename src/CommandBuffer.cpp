#include "CommandBuffer.h"

void CommandBuffer::beginSTC(vk::raii::CommandBuffer &buffer) {
	vk::CommandBufferBeginInfo commandBufferBeginInfo;
	commandBufferBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	buffer.begin(commandBufferBeginInfo);
}

void CommandBuffer::endSTC(
		vk::raii::CommandBuffer &buffer,
		vk::raii::Queue &queue
) {
	buffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.setCommandBufferCount(1);
	submitInfo.setCommandBuffers(*buffer);

	queue.submit(submitInfo);
	queue.waitIdle();
}

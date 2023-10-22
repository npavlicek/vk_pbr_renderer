#include "Util.h"

#include <vulkan/vulkan_to_string.hpp>

#include <imgui.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_win32.h>

void keyCallback(
		GLFWwindow *window,
		int key,
		int scancode,
		int action,
		int mods
) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(
				window,
				GLFW_TRUE
		);
	}
}

void beginRenderPass(
		vk::raii::CommandBuffer &commandBuffer,
		vk::raii::Framebuffer &frameBuffer,
		vk::raii::RenderPass &renderPass,
		vk::Rect2D renderArea
) {
	vk::ClearValue clearValue{vk::ClearColorValue{.15f, 0.f, .25f, 1.f}};

	vk::RenderPassBeginInfo renderPassBeginInfo;
	renderPassBeginInfo.setClearValueCount(1);
	renderPassBeginInfo.setClearValues(clearValue);
	renderPassBeginInfo.setRenderPass(*renderPass);
	renderPassBeginInfo.setFramebuffer(*frameBuffer);
	renderPassBeginInfo.setRenderArea(renderArea);

	commandBuffer.beginRenderPass(
			renderPassBeginInfo,
			vk::SubpassContents::eInline
	);
}

void endRenderPass(vk::raii::CommandBuffer &commandBuffer) {
	commandBuffer.endRenderPass();
}

void draw(
		vk::raii::CommandBuffer &commandBuffer,
		vk::raii::Pipeline &pipeline,
		vk::Extent2D swapChainExtent
) {
	commandBuffer.bindPipeline(
			vk::PipelineBindPoint::eGraphics,
			*pipeline
	);

	vk::Viewport viewport{
			0,
			0,
			static_cast<float>(swapChainExtent.width),
			static_cast<float>(swapChainExtent.height),
			0,
			1
	};
	vk::Rect2D scissor{
			{0, 0},
			swapChainExtent
	};
	commandBuffer.setScissor(
			0,
			scissor
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

void writeCommandBuffer(
		vk::raii::CommandBuffer &commandBuffer,
		vk::raii::Pipeline &pipeline,
		vk::raii::Framebuffer &frameBuffer,
		vk::raii::RenderPass &renderPass,
		vk::SurfaceCapabilitiesKHR surfaceCapabilities
) {
	vk::CommandBufferBeginInfo commandBufferBeginInfo;
	commandBuffer.begin(commandBufferBeginInfo);
	beginRenderPass(
			commandBuffer,
			frameBuffer,
			renderPass,
			{
					{},
					surfaceCapabilities.currentExtent
			}
	);
	draw(
			commandBuffer,
			pipeline,
			surfaceCapabilities.currentExtent
	);
	endRenderPass(commandBuffer);
	commandBuffer.end();
}

int main() {
	if (!glfwInit())
		throw std::runtime_error("Could not initialize GLFW");

	glfwWindowHint(
			GLFW_RESIZABLE,
			GLFW_FALSE
	);
	glfwWindowHint(
			GLFW_CLIENT_API,
			GLFW_NO_API
	);

	GLFWwindow *window = glfwCreateWindow(
			1280,
			720,
			"Testing Vulkan!",
			nullptr,
			nullptr
	);

	glfwSetKeyCallback(
			window,
			keyCallback
	);

	vk::raii::Context context;
	auto instance = util::createInstance(
			context,
			"Vulkan PBR Renderer",
			"Vulkan PBR Renderer"
	);
	auto physicalDevice = util::selectPhysicalDevice(instance);
	auto queueFamilyIndex = util::selectQueueFamily(physicalDevice);
	auto device = util::createDevice(
			physicalDevice,
			queueFamilyIndex
	);
	auto surface = util::createSurface(
			instance,
			window
	);
	auto swapChainFormat = util::selectSwapChainFormat(
			physicalDevice,
			surface
	);
	auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
	auto swapChain = util::createSwapChain(
			device,
			physicalDevice,
			surface,
			swapChainFormat,
			surfaceCapabilities
	);
	auto images = swapChain.getImages();
	auto imageViews = util::createImageViews(
			device,
			images,
			swapChainFormat
	);
	auto commandPool = util::createCommandPool(
			device,
			queueFamilyIndex
	);
	auto commandBuffers = util::createCommandBuffers(
			device,
			commandPool,
			1
	);
	auto shaderModules = util::createShaderModules(
			device,
			"shaders/vert.spv",
			"shaders/frag.spv"
	);
	auto renderPass = util::createRenderPass(
			device,
			swapChainFormat
	);
	auto pipeline = util::createPipeline(
			device,
			renderPass,
			shaderModules,
			surfaceCapabilities,
			swapChainFormat
	);
	auto frameBuffers = util::createFrameBuffers(
			device,
			renderPass,
			imageViews,
			surfaceCapabilities
	);

	auto queue = device.getQueue(
			queueFamilyIndex,
			0
	);

	vk::SemaphoreCreateInfo semaphoreCreateInfo;
	auto imageAvailableSemaphore = device.createSemaphore(semaphoreCreateInfo);
	auto renderFinishedSempahore = device.createSemaphore(semaphoreCreateInfo);

	vk::FenceCreateInfo fenceCreateInfo{
			vk::FenceCreateFlagBits::eSignaled
	};
	auto fence = device.createFence(fenceCreateInfo);

	while (!glfwWindowShouldClose(window)) {
		auto res = device.waitForFences(
				*fence,
				VK_TRUE,
				UINT32_MAX
		);
		device.resetFences(*fence);

		uint32_t imageIndex;
		std::tie(
				res,
				imageIndex
		) = swapChain.acquireNextImage(
				UINT64_MAX,
				*imageAvailableSemaphore,
				VK_NULL_HANDLE
		);

		commandBuffers[0].reset();
		writeCommandBuffer(
				commandBuffers[0],
				pipeline,
				frameBuffers[imageIndex],
				renderPass,
				surfaceCapabilities
		);

		std::vector<vk::PipelineStageFlags> pipelineStageFlags{vk::PipelineStageFlagBits::eColorAttachmentOutput};

		vk::SubmitInfo submitInfo;
		submitInfo.setWaitSemaphoreCount(1);
		submitInfo.setWaitSemaphores(*imageAvailableSemaphore);
		submitInfo.setWaitDstStageMask(pipelineStageFlags);
		submitInfo.setCommandBufferCount(1);
		submitInfo.setCommandBuffers(*commandBuffers[0]);
		submitInfo.setSignalSemaphoreCount(1);
		submitInfo.setSignalSemaphores(*renderFinishedSempahore);

		queue.submit(
				submitInfo,
				*fence
		);

		vk::PresentInfoKHR presentInfo;
		presentInfo.setSwapchainCount(1);
		presentInfo.setSwapchains(*swapChain);
		presentInfo.setImageIndices(imageIndex);
		presentInfo.setWaitSemaphoreCount(1);
		presentInfo.setWaitSemaphores(*renderFinishedSempahore);

		res = queue.presentKHR(presentInfo);

		glfwPollEvents();
	}
	device.waitIdle();

	glfwTerminate();
	return 0;
}

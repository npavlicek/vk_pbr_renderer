#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>

namespace N
{
struct RenderPassCreateInfo
{
	vk::Device &device;
	vk::SampleCountFlagBits &samples;
	vk::Format &surfaceFormat;
	vk::Format &depthFormat;
};

class RenderPass
{
  public:
	constexpr RenderPass(RenderPass &rhs) = delete;
	constexpr RenderPass &operator=(RenderPass &rhs) = delete;
	constexpr RenderPass(RenderPass &&rhs) = default;
	RenderPass &operator=(RenderPass &&rhs) = default;

	void create(const RenderPassCreateInfo &createInfo);
	void destroy(const vk::Device &device);

	const vk::RenderPass &get()
	{
		return renderPass;
	}

  private:
	vk::RenderPass renderPass;
};
} // namespace N

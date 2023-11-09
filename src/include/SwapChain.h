#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace N
{
struct SwapChainCreateInfo
{
	vk::Device device;
	vk::SurfaceKHR surface;
	vk::PhysicalDevice physicalDevice;
	uint32_t framesInFlight;
};

class SwapChain
{
  public:
	constexpr SwapChain(SwapChain &rhs) = delete;
	constexpr SwapChain &operator=(SwapChain &rhs) = delete;
	constexpr SwapChain(SwapChain &&rhs) = default;
	SwapChain &operator=(SwapChain &&rhs) = default;

	void create(const SwapChainCreateInfo &createInfo);
	void destroy(const vk::Device &device);

	const vk::SwapchainKHR &getSwapChain() const
	{
		return swapChain;
	}

	const vk::SurfaceFormatKHR &getSurfaceFormat() const
	{
		return surfaceFormat;
	}

	const vk::PresentModeKHR &getPresentMode() const
	{
		return presentMode;
	}

  private:
	vk::SwapchainKHR swapChain;
	vk::SurfaceFormatKHR surfaceFormat;
	vk::PresentModeKHR presentMode;

	void selectFormat(const SwapChainCreateInfo &createInfo);
	void selectPresentMode(const SwapChainCreateInfo &createInfo);
};
} // namespace N

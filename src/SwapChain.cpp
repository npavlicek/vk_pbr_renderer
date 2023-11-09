#include "SwapChain.h"
#include <optional>
#include <stdexcept>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>
#include <xutility>

namespace N
{
void SwapChain::create(const SwapChainCreateInfo &createInfo)
{
	selectFormat(createInfo);
	selectPresentMode(createInfo);

	auto surfaceCapabilities = createInfo.physicalDevice.getSurfaceCapabilitiesKHR(createInfo.surface);

	vk::SwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.setSurface(createInfo.surface);
	swapChainCreateInfo.setMinImageCount(createInfo.framesInFlight);
	swapChainCreateInfo.setImageFormat(surfaceFormat.format);
	swapChainCreateInfo.setImageColorSpace(surfaceFormat.colorSpace);
	swapChainCreateInfo.setImageExtent(surfaceCapabilities.currentExtent);
	swapChainCreateInfo.setImageArrayLayers(1);
	swapChainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
	swapChainCreateInfo.setImageSharingMode(vk::SharingMode::eExclusive);
	swapChainCreateInfo.setPreTransform(surfaceCapabilities.currentTransform);
	swapChainCreateInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
	swapChainCreateInfo.setPresentMode(presentMode);
	swapChainCreateInfo.setClipped(vk::True);

	swapChain = createInfo.device.createSwapchainKHR(swapChainCreateInfo);

	swapChainImages = createInfo.device.getSwapchainImagesKHR(swapChain);

	vk::ImageSubresourceRange sr;
	sr.setAspectMask(vk::ImageAspectFlagBits::eColor);
	sr.setBaseArrayLayer(0);
	sr.setBaseMipLevel(0);
	sr.setLevelCount(1);
	sr.setLayerCount(1);

	vk::ImageViewCreateInfo imageViewCreateInfo;
	imageViewCreateInfo.setViewType(vk::ImageViewType::e2D);
	imageViewCreateInfo.setComponents(vk::ComponentSwizzle{});
	imageViewCreateInfo.setFormat(surfaceFormat.format);
	imageViewCreateInfo.setSubresourceRange(sr);

	for (const auto &image : swapChainImages)
	{
		imageViewCreateInfo.setImage(image);
		swapChainImageViews.push_back(createInfo.device.createImageView(imageViewCreateInfo));
	}
}

void SwapChain::destroy(const vk::Device &device)
{
	device.destroySwapchainKHR(swapChain);
}

void SwapChain::selectPresentMode(const SwapChainCreateInfo &createInfo)
{
	auto presentModes = createInfo.physicalDevice.getSurfacePresentModesKHR(createInfo.surface);

	std::optional<vk::PresentModeKHR> selectedPresentMode;
	for (const auto &mode : presentModes)
	{
		if (mode == vk::PresentModeKHR::eMailbox)
		{
			selectedPresentMode = mode;
			break;
		}
	}

	if (!selectedPresentMode.has_value())
	{
		throw std::runtime_error("requested surface present mode not supported!");
	}

	presentMode = selectedPresentMode.value();
}

void SwapChain::selectFormat(const SwapChainCreateInfo &createInfo)
{
	auto formats = createInfo.physicalDevice.getSurfaceFormatsKHR(createInfo.surface);

	std::optional<vk::SurfaceFormatKHR> selectedSurfaceFormat;
	for (const auto &format : formats)
	{
		if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			selectedSurfaceFormat = format;
			break;
		}
	}

	if (!selectedSurfaceFormat.has_value())
	{
		throw std::runtime_error("requested surface format not supported");
	}

	surfaceFormat = selectedSurfaceFormat.value();
}

} // namespace N

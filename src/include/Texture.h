#pragma once

#include "vulkan/vulkan_raii.hpp"

#include "stb_image.h"

#include "Util.h"
#include "CommandBuffer.h"

#include <string>

class Texture
{
public:
	Texture() = delete;
	Texture(
		vk::raii::Device &device,
		vk::raii::PhysicalDevice &physicalDevice,
		vk::raii::CommandBuffer &commandBuffer,
		vk::raii::Queue &queue,
		const std::string &path);
	Texture(const Texture &other) = delete;
	Texture(const Texture &&other) = delete;

	vk::ImageView getImageView() const
	{
		return **imageView;
	}

	vk::Sampler getSampler() const
	{
		return **imageSampler;
	}

private:
	std::unique_ptr<vk::raii::Image> image;
	std::unique_ptr<vk::raii::DeviceMemory> imageMemory;
	std::unique_ptr<vk::raii::ImageView> imageView;
	std::unique_ptr<vk::raii::Sampler> imageSampler;

	int width, height, channels;
	vk::DeviceSize imageSize;

	void createSampler(
		vk::raii::Device &device,
		float maxAnisotropy);
	void copyBufferToImage(
		vk::raii::CommandBuffer &buffer,
		vk::raii::Buffer &stagingBuffer);
	void transitionImage(
		vk::raii::CommandBuffer &buffer,
		vk::ImageLayout oldLayout,
		vk::ImageLayout newLayout);
};


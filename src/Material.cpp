#include "Material.h"

#include "stb_image.h"

#include "CommandBuffer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace N
{
Material::Material(const VmaAllocator &allocator, const vk::Device &device, const vk::Queue &queue,
				   const vk::CommandBuffer &commandBuffer, const tinyobj::material_t &tinyObjMat)
{
	std::tie(diffuse, diffuseAlloc) =
		loadImage(allocator, queue, commandBuffer, vk::Format::eR8G8B8A8Srgb, tinyObjMat.diffuse_texname.c_str());

	std::tie(metallic, metallicAlloc) =
		loadImage(allocator, queue, commandBuffer, vk::Format::eR8G8B8A8Srgb, tinyObjMat.metallic_texname.c_str());

	std::tie(roughness, roughnessAlloc) =
		loadImage(allocator, queue, commandBuffer, vk::Format::eR8G8B8A8Srgb, tinyObjMat.roughness_texname.c_str());

	std::tie(normal, normalAlloc) =
		loadImage(allocator, queue, commandBuffer, vk::Format::eR8G8B8A8Srgb, tinyObjMat.normal_texname.c_str());

	VkImageSubresourceRange imageSubresourceRange{};
	imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageSubresourceRange.baseArrayLayer = 0;
	imageSubresourceRange.baseMipLevel = 0;
	imageSubresourceRange.layerCount = 1;
	imageSubresourceRange.levelCount = mipLevels;

	VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.components = VkComponentMapping{};
	imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageViewCreateInfo.subresourceRange = imageSubresourceRange;

	imageViewCreateInfo.image = diffuse;
	auto res = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &diffuseView);
	vk::resultCheck(vk::Result(res), "Could not create image view!");

	imageViewCreateInfo.image = metallic;
	res = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &metallicView);
	vk::resultCheck(vk::Result(res), "Could not create image view!");

	imageViewCreateInfo.image = roughness;
	res = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &roughnessView);
	vk::resultCheck(vk::Result(res), "Could not create image view!");

	// imageViewCreateInfo.format = static_cast<VkFormat>(vk::Format::eR8G8B8A8Unorm);

	imageViewCreateInfo.image = normal;
	res = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &normalView);
	vk::resultCheck(vk::Result(res), "Could not create image view!");
}

void Material::destroy(const VmaAllocator &allocator, const vk::Device &device,
					   const vk::DescriptorPool &descriptorPool)
{
	vkDestroyImageView(device, diffuseView, nullptr);
	vkDestroyImageView(device, metallicView, nullptr);
	vkDestroyImageView(device, roughnessView, nullptr);
	vkDestroyImageView(device, normalView, nullptr);

	vmaDestroyImage(allocator, diffuse, diffuseAlloc);
	vmaDestroyImage(allocator, metallic, metallicAlloc);
	vmaDestroyImage(allocator, roughness, roughnessAlloc);
	vmaDestroyImage(allocator, normal, normalAlloc);
}

std::tuple<VkImage, VmaAllocation> Material::loadImage(const VmaAllocator &allocator, const vk::Queue &queue,
													   const vk::CommandBuffer &commandBuffer, vk::Format format,
													   const char *path)
{
	int loadedWidth, loadedHeight, channels;
	width = loadedWidth;
	height = loadedHeight;
	unsigned char *data = stbi_load(path, &loadedWidth, &loadedHeight, &channels, 4);

	if (!data)
	{
		throw std::runtime_error(std::string("Failed to load image: ").append(path));
	}

	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))) + 1);

	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	// FIXME: not sure if this is right
	bufferCreateInfo.size = width * height * 4;

	VmaAllocationCreateInfo allocationCreateInfo{};
	allocationCreateInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT |
								 VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;

	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VmaAllocationInfo allocInfo;

	auto res = vmaCreateBuffer(allocator, &bufferCreateInfo, &allocationCreateInfo, &stagingBuffer, &stagingAllocation,
							   &allocInfo);
	vk::resultCheck(vk::Result(res), "Could not create buffer!");

	memcpy(allocInfo.pMappedData, data, bufferCreateInfo.size);

	VkExtent3D extent;
	extent.depth = 1;
	extent.width = width;
	extent.height = height;

	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.extent = extent;
	imageCreateInfo.format = static_cast<VkFormat>(format);
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.mipLevels = mipLevels;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	VmaAllocationCreateInfo imageAllocationCreateInfo{};
	imageAllocationCreateInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	imageAllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	VkImage image;
	VmaAllocation imageAllocation;

	res = vmaCreateImage(allocator, &imageCreateInfo, &imageAllocationCreateInfo, &image, &imageAllocation, nullptr);
	vk::resultCheck(vk::Result(res), "Could not create buffer!");

	// Transition to transfer dst

	VkImageSubresourceRange imageSubresourceRange{};
	imageSubresourceRange.baseArrayLayer = 0;
	imageSubresourceRange.baseMipLevel = 0;
	imageSubresourceRange.layerCount = 1;
	imageSubresourceRange.levelCount = mipLevels;
	imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkImageMemoryBarrier imageMemoryBarrier{};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_NONE;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.subresourceRange = imageSubresourceRange;

	CommandBuffer::beginSTC(commandBuffer);

	commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
								  vk::DependencyFlagBits::eByRegion, nullptr, nullptr,
								  vk::ImageMemoryBarrier(imageMemoryBarrier));

	CommandBuffer::endSTC(commandBuffer, queue);

	//

	// Copy image data
	CommandBuffer::beginSTC(commandBuffer);

	vk::ImageSubresourceLayers imageSubresourceLayers{};
	imageSubresourceLayers.setMipLevel(0);
	imageSubresourceLayers.setLayerCount(1);
	imageSubresourceLayers.setBaseArrayLayer(0);
	imageSubresourceLayers.setAspectMask(vk::ImageAspectFlagBits::eColor);

	vk::BufferImageCopy bufferImageCopy{};
	bufferImageCopy.setImageSubresource(imageSubresourceLayers);
	bufferImageCopy.setImageExtent(vk::Extent3D(width, height, 1));
	bufferImageCopy.setImageOffset(vk::Offset3D(0, 0, 0));

	commandBuffer.copyBufferToImage(stagingBuffer, image, vk::ImageLayout::eTransferDstOptimal, bufferImageCopy);

	CommandBuffer::endSTC(commandBuffer, queue);
	//

	// Transfer from transfer dst to shader optimal
	// Commented out because we do this while generating mip maps
	/* CommandBuffer::beginSTC(commandBuffer);

	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
								  vk::DependencyFlagBits::eByRegion, 0, 0, vk::ImageMemoryBarrier(imageMemoryBarrier));

	CommandBuffer::endSTC(commandBuffer, queue); */
	//

	generateMipMaps(queue, commandBuffer, image);

	free(reinterpret_cast<void *>(data));

	vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);

	return std::make_tuple(image, imageAllocation);
}

void Material::generateMipMaps(const vk::Queue &queue, const vk::CommandBuffer &commandBuffer, const vk::Image &image)
{
	CommandBuffer::beginSTC(commandBuffer);

	vk::ImageSubresourceRange isr{};
	isr.setAspectMask(vk::ImageAspectFlagBits::eColor);
	isr.setLayerCount(1);
	isr.setLevelCount(mipLevels);
	isr.setBaseArrayLayer(0);

	vk::ImageMemoryBarrier barrier{};
	barrier.setImage(image);
	barrier.setNewLayout(vk::ImageLayout::eTransferSrcOptimal);
	barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
	barrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);
	barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);

	int32_t mipWidth = width;
	int32_t mipHeight = height;

	for (uint32_t i = 1; i < mipLevels; i++)
	{
		isr.setBaseMipLevel(i - 1);
		barrier.setSubresourceRange(isr);

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
									  vk::DependencyFlagBits::eByRegion, nullptr, nullptr, barrier);

		vk::ImageBlit blit{};

		std::array<vk::Offset3D, 2> srcOffset;
		srcOffset[0] = vk::Offset3D{0, 0, 0};
		srcOffset[1] = vk::Offset3D{mipWidth, mipHeight, 1};
		blit.setSrcOffsets(srcOffset);

		vk::ImageSubresourceLayers srcLayers{};
		srcLayers.setMipLevel(i - 1);
		srcLayers.setLayerCount(1);
		srcLayers.setBaseArrayLayer(0);
		srcLayers.setAspectMask(vk::ImageAspectFlagBits::eColor);
		blit.setSrcSubresource(srcLayers);

		std::array<vk::Offset3D, 2> dstOffset;
		dstOffset[0] = vk::Offset3D{0, 0, 0};
		dstOffset[1] = vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
		blit.setDstOffsets(dstOffset);

		vk::ImageSubresourceLayers dstLayers{};
		dstLayers.setAspectMask(vk::ImageAspectFlagBits::eColor);
		dstLayers.setBaseArrayLayer(0);
		dstLayers.setLayerCount(1);
		dstLayers.setMipLevel(i);

		commandBuffer.blitImage(image, vk::ImageLayout::eTransferDstOptimal, image,
								vk::ImageLayout::eTransferSrcOptimal, blit, vk::Filter::eLinear);

		barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal);
		barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
		barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead);
		barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
									  vk::DependencyFlagBits::eByRegion, nullptr, nullptr, barrier);

		if (mipWidth > 1)
			mipWidth /= 2;
		if (mipHeight > 1)
			mipHeight /= 2;
	}

	isr.setBaseMipLevel(mipLevels - 1);
	barrier.setSubresourceRange(isr);
	barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead);
	barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
	barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal);
	barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

	commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
								  vk::DependencyFlagBits::eByRegion, nullptr, nullptr, barrier);

	CommandBuffer::endSTC(commandBuffer, queue);
}

vk::Sampler Material::createSampler(const vk::Device &device, float maxAnisotropy)
{
	vk::SamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.setAddressModeU(vk::SamplerAddressMode::eClampToEdge);
	samplerCreateInfo.setAddressModeV(vk::SamplerAddressMode::eClampToEdge);
	samplerCreateInfo.setAddressModeW(vk::SamplerAddressMode::eClampToEdge);
	samplerCreateInfo.setBorderColor(vk::BorderColor::eIntOpaqueBlack);
	samplerCreateInfo.setAnisotropyEnable(vk::True);
	samplerCreateInfo.setCompareEnable(vk::False);
	samplerCreateInfo.setCompareOp(vk::CompareOp::eAlways);
	samplerCreateInfo.setUnnormalizedCoordinates(vk::False);
	samplerCreateInfo.setMipmapMode(vk::SamplerMipmapMode::eLinear);
	samplerCreateInfo.setMagFilter(vk::Filter::eLinear);
	samplerCreateInfo.setMinFilter(vk::Filter::eLinear);
	samplerCreateInfo.setMaxAnisotropy(maxAnisotropy);
	samplerCreateInfo.setMaxLod(mipLevels);
	samplerCreateInfo.setMinLod(0.f);

	return device.createSampler(samplerCreateInfo);
}

void Material::bind(const vk::CommandBuffer &commandBuffer, const vk::PipelineLayout &pipelineLayout) const
{
	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, descriptorSets, nullptr);
}

void Material::createDescriptorSets(const vk::Device &device, const vk::DescriptorPool &pool,
									const vk::DescriptorSetLayout &setLayout, const vk::Sampler &sampler)
{
	vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{};
	descriptorSetAllocateInfo.setDescriptorPool(pool);
	descriptorSetAllocateInfo.setSetLayouts(setLayout);

	descriptorSets = device.allocateDescriptorSets(descriptorSetAllocateInfo);

	vk::DescriptorImageInfo diffuseImageInfo;
	diffuseImageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	diffuseImageInfo.setSampler(sampler);
	diffuseImageInfo.setImageView(diffuseView);

	vk::DescriptorImageInfo metallicImageInfo;
	metallicImageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	metallicImageInfo.setSampler(sampler);
	metallicImageInfo.setImageView(metallicView);

	vk::DescriptorImageInfo roughnessImageInfo;
	roughnessImageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	roughnessImageInfo.setSampler(sampler);
	roughnessImageInfo.setImageView(roughnessView);

	vk::DescriptorImageInfo normalImageInfo;
	normalImageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	normalImageInfo.setSampler(sampler);
	normalImageInfo.setImageView(normalView);

	vk::WriteDescriptorSet writeDiffuse{};
	writeDiffuse.setDescriptorCount(1);
	writeDiffuse.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	writeDiffuse.setDstArrayElement(0);
	writeDiffuse.setDstBinding(0);
	writeDiffuse.setDstSet(descriptorSets.at(0));
	writeDiffuse.setImageInfo(diffuseImageInfo);

	vk::WriteDescriptorSet writeMetallic = writeDiffuse;
	writeMetallic.setDstBinding(1);
	writeMetallic.setDstSet(descriptorSets.at(0));
	writeMetallic.setImageInfo(metallicImageInfo);

	vk::WriteDescriptorSet writeRoughness = writeDiffuse;
	writeRoughness.setDstBinding(2);
	writeRoughness.setDstSet(descriptorSets.at(0));
	writeRoughness.setImageInfo(roughnessImageInfo);

	vk::WriteDescriptorSet writeNormal = writeDiffuse;
	writeNormal.setDstBinding(3);
	writeNormal.setDstSet(descriptorSets.at(0));
	writeNormal.setImageInfo(normalImageInfo);

	std::array<vk::WriteDescriptorSet, 4> writeDescriptorSets{writeDiffuse, writeMetallic, writeRoughness, writeNormal};

	device.updateDescriptorSets(writeDescriptorSets, nullptr);
}
} // namespace N

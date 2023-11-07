#include "Material.h"

#include "stb_image.h"

#include "CommandBuffer.h"

#include <format>

Material::Material(const VmaAllocator &allocator, const vk::Queue &queue, const vk::CommandBuffer &commandBuffer, const tinyobj::material_t &tinyObjMat)
{
	std::tie(
		diffuse,
		diffuseAlloc) = loadImage(allocator, queue, commandBuffer, tinyObjMat.diffuse_texname.c_str());

	std::tie(
		metallic,
		metallicAlloc) = loadImage(allocator, queue, commandBuffer, tinyObjMat.metallic_texname.c_str());

	std::tie(
		roughness,
		roughnessAlloc) = loadImage(allocator, queue, commandBuffer, tinyObjMat.roughness_texname.c_str());

	std::tie(
		normal,
		normalAlloc) = loadImage(allocator, queue, commandBuffer, tinyObjMat.normal_texname.c_str());
}

void Material::destroy(VmaAllocator allocator)
{
	vmaDestroyImage(allocator, diffuse, diffuseAlloc);
	vmaDestroyImage(allocator, metallic, metallicAlloc);
	vmaDestroyImage(allocator, roughness, roughnessAlloc);
	vmaDestroyImage(allocator, normal, normalAlloc);
}

std::tuple<VkImage, VmaAllocation> Material::loadImage(const VmaAllocator &allocator, const vk::Queue &queue, const vk::CommandBuffer &commandBuffer, const char *path)
{
	int width, height, channels;
	unsigned char *data = stbi_load(path, &width, &height, &channels, 4);

	if (!data)
	{
		throw std::runtime_error(std::format("Could not open material texture: %s", path));
	}

	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	// FIXME: not sure if this is right
	bufferCreateInfo.size = width * height * 4;

	VmaAllocationCreateInfo allocationCreateInfo{};
	allocationCreateInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT | VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;

	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VmaAllocationInfo allocInfo;

	vmaCreateBuffer(allocator, &bufferCreateInfo, &allocationCreateInfo, &stagingBuffer, &stagingAllocation, &allocInfo);

	memcpy(allocInfo.pMappedData, data, bufferCreateInfo.size);

	VkExtent3D extent;
	extent.depth = 1;
	extent.width = width;
	extent.height = height;

	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.extent = extent;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	VmaAllocationCreateInfo imageAllocationCreateInfo{};
	imageAllocationCreateInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	imageAllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	VkImage image;
	VmaAllocation imageAllocation;

	vmaCreateImage(allocator, &imageCreateInfo, &imageAllocationCreateInfo, &image, &imageAllocation, nullptr);

	// Transition to transfer dst

	VkImageSubresourceRange imageSubresourceRange{};
	imageSubresourceRange.baseArrayLayer = 0;
	imageSubresourceRange.baseMipLevel = 0;
	imageSubresourceRange.layerCount = 1;
	imageSubresourceRange.levelCount = 1;
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

	commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlagBits::eByRegion, nullptr, nullptr, vk::ImageMemoryBarrier(imageMemoryBarrier));

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
	CommandBuffer::beginSTC(commandBuffer);

	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlagBits::eByRegion, 0, 0, vk::ImageMemoryBarrier(imageMemoryBarrier));

	CommandBuffer::endSTC(commandBuffer, queue);
	//

	free(reinterpret_cast<void *>(data));

	vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);

	return std::make_tuple(image, imageAllocation);
}

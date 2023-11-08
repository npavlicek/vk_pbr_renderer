#pragma once

#include <tuple>

#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

#include "tiny_obj_loader.h"

class Material
{
public:
	constexpr Material() = delete;
	Material(const VmaAllocator &allocator, const vk::Device &device, const vk::Queue &queue, const vk::CommandBuffer &commandBuffer, const tinyobj::material_t &tinyObjMat);
	constexpr Material(const Material &) = delete;
	constexpr Material &operator=(const Material &rhs) = delete;
	constexpr Material(Material &&) = default;

	void destroy(const VmaAllocator &allocator, const vk::Device &device);

	static VkSampler createSampler(const vk::Device &device);

private:
	std::tuple<VkImage, VmaAllocation>
	loadImage(const VmaAllocator &allocator, const vk::Queue &queue, const vk::CommandBuffer &commandBuffer, const char *path);

	VkImage diffuse;
	VkImageView diffuseView;
	VmaAllocation diffuseAlloc;

	VkImage metallic;
	VkImageView metallicView;
	VmaAllocation metallicAlloc;

	VkImage roughness;
	VkImageView roughnessView;
	VmaAllocation roughnessAlloc;

	VkImage normal;
	VkImageView normalView;
	VmaAllocation normalAlloc;
};

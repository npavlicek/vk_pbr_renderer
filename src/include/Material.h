#pragma once

#include <tuple>

#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

#include "tiny_obj_loader.h"

class Material
{
public:
	constexpr Material() = delete;
	Material(const VmaAllocator &allocator, const vk::Queue &queue, const vk::CommandBuffer &commandBuffer, const tinyobj::material_t &tinyObjMat);
	constexpr Material(const Material &) = delete;
	constexpr Material &operator=(const Material &rhs) = delete;
	constexpr Material(Material &&) = default;

	void destroy(VmaAllocator allocator);

private:
	std::tuple<VkImage, VmaAllocation> loadImage(const VmaAllocator &allocator, const vk::Queue &queue, const vk::CommandBuffer &commandBuffer, const char *path);

	VkImage diffuse;
	VmaAllocation diffuseAlloc;

	VkImage metallic;
	VmaAllocation metallicAlloc;

	VkImage roughness;
	VmaAllocation roughnessAlloc;

	VkImage normal;
	VmaAllocation normalAlloc;
};

#pragma once

#include <tuple>

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "tiny_obj_loader.h"

namespace N
{
class Material
{
  public:
	constexpr Material() = delete;
	Material(const VmaAllocator &allocator, const vk::Device &device, const vk::Queue &queue,
			 const vk::CommandBuffer &commandBuffer, const tinyobj::material_t &tinyObjMat);
	constexpr Material(const Material &) = delete;
	constexpr Material &operator=(const Material &rhs) = delete;
	constexpr Material(Material &&) = default;
	Material &operator=(Material &&) = default;

	void bind(const vk::CommandBuffer &commandBuffer, const vk::PipelineLayout &pipelineLayout) const;
	void destroy(const VmaAllocator &allocator, const vk::Device &device, const vk::DescriptorPool &descriptorPool);

	static vk::Sampler createSampler(const vk::Device &device, float maxAnisotropy);
	void createDescriptorSets(const vk::Device &device, const vk::DescriptorPool &pool,
							  const vk::DescriptorSetLayout &setLayout, const vk::Sampler &sampler);

  private:
	std::tuple<VkImage, VmaAllocation> loadImage(const VmaAllocator &allocator, const vk::Queue &queue,
												 const vk::CommandBuffer &commandBuffer, vk::Format format,
												 const char *path);

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

	std::vector<vk::DescriptorSet> descriptorSets;
};
} // namespace N

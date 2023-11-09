#pragma once

#include <iostream>

#include <vector>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "Material.h"
#include "Mesh.h"

class Model
{
  public:
	constexpr Model() = delete;
	Model(const VmaAllocator &vmaAllocator, const vk::Device &device, const vk::Queue &queue,
		  const vk::CommandBuffer &commandBuffer, const vk::DescriptorPool &descriptorPool,
		  const vk::DescriptorSetLayout &descriptorSetLayout, const vk::Sampler &sampler, const char *path);
	constexpr Model(const Model &) = delete;
	constexpr Model &operator=(const Model &) = delete;
	constexpr Model(Model &&) = default;

	const std::vector<Mesh> &getMeshes() const
	{
		return meshes;
	}

	const glm::mat4 &getModel() const
	{
		return model;
	}

	void setModel(const glm::mat4 model)
	{
		this->model = model;
	}

	void draw(const vk::CommandBuffer &commandBuffer, const vk::DescriptorSet &additionalSet,
			  const vk::PipelineLayout &pipelineLayout) const;
	void destroy(const VmaAllocator &vmaAllocator, const vk::Device &device, const vk::DescriptorPool &descriptorPool);

  private:
	std::vector<Mesh> meshes;
	std::vector<Material> materials;
	glm::mat4 model{1.f};

	void uploadMeshes(const VmaAllocator &vmaAllocator, const vk::Queue &queue, const vk::CommandBuffer &commandBuffer);
};

#pragma once

#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include "Material.h"
#include "Mesh.h"

namespace N
{
struct ModelCreateInfo
{
	VmaAllocator vmaAllocator;
	vk::Device device;
	vk::Queue queue;
	vk::CommandBuffer commandBuffer;
	vk::DescriptorPool descriptorPool;
	vk::DescriptorSetLayout descriptorSetLayout;
	vk::Sampler sampler;
};

class Model
{
  public:
	constexpr Model() = delete;
	Model(const ModelCreateInfo &createInfo, const char *path);
	constexpr Model(const Model &) = delete;
	constexpr Model &operator=(const Model &) = delete;
	constexpr Model(Model &&) = default;
	Model &operator=(Model &&) = default;

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

	void draw(const vk::CommandBuffer &commandBuffer, const vk::PipelineLayout &pipelineLayout) const;
	void destroy(const VmaAllocator &vmaAllocator, const vk::Device &device, const vk::DescriptorPool &descriptorPool);

  private:
	std::vector<Mesh> meshes;
	std::vector<Material> materials;
	glm::mat4 model{1.f};

	void uploadMeshes(const ModelCreateInfo &createInfo);
};
} // namespace N

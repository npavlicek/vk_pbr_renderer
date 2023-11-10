#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include "tiny_obj_loader.h"

#include "Vertex.h"

class Mesh
{
  public:
	constexpr Mesh() = delete;
	Mesh(const tinyobj::shape_t &shape, const tinyobj::attrib_t &attrib, int materialId);
	constexpr Mesh(const Mesh &) = delete;
	constexpr Mesh &operator=(const Mesh &) = delete;
	constexpr Mesh(Mesh &&) = default;

	const std::vector<Vertex> &getVertices() const
	{
		return vertices;
	}
	const std::vector<uint16_t> &getIndices() const
	{
		return indices;
	}
	int getMaterialId() const
	{
		return materialId;
	}

	void draw(const vk::CommandBuffer &commandBuffer) const;
	void uploadMesh(const VmaAllocator &vmaAllocator, const vk::Queue &queue, const vk::CommandBuffer &commandBuffer);
	void destroy(const VmaAllocator &vmaAllocator);

  private:
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
	int materialId;

	vk::Buffer vertexBuffer;
	VmaAllocation vertexBufferAllocation;

	vk::Buffer indexBuffer;
	VmaAllocation indexBufferAllocation;

	void bind(const vk::CommandBuffer &commandBuffer) const;
};

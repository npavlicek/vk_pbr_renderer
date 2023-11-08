#pragma once

#include <random>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include "tiny_obj_loader.h"
#include "CommandBuffer.h"

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoords;

	bool operator==(const Vertex &other) const
	{
		return pos == other.pos && color == other.color && texCoords == other.texCoords;
	}

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		vk::VertexInputBindingDescription vertexInputBindingDescription;
		vertexInputBindingDescription.setBinding(0);
		vertexInputBindingDescription.setStride(sizeof(Vertex));
		vertexInputBindingDescription.setInputRate(vk::VertexInputRate::eVertex);

		return vertexInputBindingDescription;
	}

	static std::vector<vk::VertexInputAttributeDescription> getAttributeDescription()
	{
		std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescriptions;
		vertexAttributeDescriptions.emplace_back(
			0,
			0,
			vk::Format::eR32G32B32Sfloat,
			offsetof(
				Vertex,
				pos));

		vertexAttributeDescriptions.emplace_back(
			1,
			0,
			vk::Format::eR32G32B32Sfloat,
			offsetof(
				Vertex,
				color));

		vertexAttributeDescriptions.emplace_back(
			2,
			0,
			vk::Format::eR32G32Sfloat,
			offsetof(
				Vertex,
				texCoords));

		return vertexAttributeDescriptions;
	}
};

namespace std
{
	template <>
	struct hash<Vertex>
	{
		size_t operator()(const Vertex &vertex) const
		{
			return (
					   (hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				   (hash<glm::vec2>()(vertex.texCoords) << 1);
		}
	};
}

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
	const std::vector<uint16_t> &getIndices() const { return indices; }
	const int getMaterialId() const { return materialId; }

	void draw(const vk::CommandBuffer &commandBuffer) const;
	void uploadMesh(const VmaAllocator &vmaAllocator, const vk::Queue &queue, const vk::CommandBuffer &commandBuffer);
	void destroy(const VmaAllocator &vmaAllocator);

private:
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
	int materialId;

	VkBuffer vertexBuffer;
	VmaAllocation vertexBufferAllocation;

	VkBuffer indexBuffer;
	VmaAllocation indexBufferAllocation;

	void bind(const vk::CommandBuffer &commandBuffer) const;
};

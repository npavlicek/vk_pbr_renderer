#pragma once

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoords;
	glm::vec3 normal;
	glm::vec3 tangent;

	bool operator==(const Vertex &other) const
	{
		return pos == other.pos && color == other.color && texCoords == other.texCoords && normal == other.normal;
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
		vertexAttributeDescriptions.emplace_back(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos));
		vertexAttributeDescriptions.emplace_back(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color));
		vertexAttributeDescriptions.emplace_back(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoords));
		vertexAttributeDescriptions.emplace_back(3, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal));
		vertexAttributeDescriptions.emplace_back(4, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, tangent));

		return vertexAttributeDescriptions;
	}
};

namespace std
{
template <> struct hash<Vertex>
{
	size_t operator()(const Vertex &vertex) const
	{
		return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
			   ((hash<glm::vec2>()(vertex.texCoords) << 1) ^ (hash<glm::vec3>()(vertex.normal) << 2) >> 1);
	}
};
} // namespace std

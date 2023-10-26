#ifndef VK_PBR_RENDERER_VERTEX_H
#define VK_PBR_RENDERER_VERTEX_H

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoords;

	static vk::VertexInputBindingDescription getBindingDescription() {
		vk::VertexInputBindingDescription vertexInputBindingDescription;
		vertexInputBindingDescription.setBinding(0);
		vertexInputBindingDescription.setStride(sizeof(Vertex));
		vertexInputBindingDescription.setInputRate(vk::VertexInputRate::eVertex);

		return vertexInputBindingDescription;
	}

	static std::vector<vk::VertexInputAttributeDescription> getAttributeDescription() {
		std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescriptions;
		vertexAttributeDescriptions.emplace_back(
				0,
				0,
				vk::Format::eR32G32B32Sfloat,
				offsetof(
						Vertex,
						pos
				)
		);

		vertexAttributeDescriptions.emplace_back(
				1,
				0,
				vk::Format::eR32G32B32Sfloat,
				offsetof(
						Vertex,
						color
				)
		);

		vertexAttributeDescriptions.emplace_back(
				2,
				0,
				vk::Format::eR32G32Sfloat,
				offsetof(
						Vertex,
						texCoords
				)
		);

		return vertexAttributeDescriptions;
	}
};

#endif //VK_PBR_RENDERER_VERTEX_H

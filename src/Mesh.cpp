#include "Mesh.h"

Mesh::Mesh(const tinyobj::shape_t &shape, const tinyobj::attrib_t &attrib)
{
	std::unordered_map<Vertex, uint16_t> uniqueVertices;

	srand(time(NULL));

	for (const auto &index : shape.mesh.indices)
	{
		Vertex vertex;
		vertex.pos[0] = attrib.vertices.at(3 * index.vertex_index);
		vertex.pos[1] = attrib.vertices.at(3 * index.vertex_index + 1);
		vertex.pos[2] = attrib.vertices.at(3 * index.vertex_index + 2);

		vertex.color[0] = static_cast<float>(std::rand()) / RAND_MAX;
		vertex.color[1] = static_cast<float>(std::rand()) / RAND_MAX;
		vertex.color[2] = static_cast<float>(std::rand()) / RAND_MAX;

		// vertex.color = glm::vec3(0.5f);

		vertex.texCoords = glm::vec2(0.f);

		// if (attrib.texcoords.size() != 0)
		// {

		// 	vertex.texCoords[0] = attrib.texcoords.at(2 * index.texcoord_index);
		// 	// REMEMBER THE Y AXIS IS FLIPPED IN VULKAN
		// 	vertex.texCoords[1] = 1 - attrib.texcoords.at(2 * index.texcoord_index + 1);
		// }
		// else
		// {
		// 	vertex.texCoords = glm::vec2(0.f);
		// }

		if (uniqueVertices.count(vertex) == 0)
		{
			uniqueVertices[vertex] = static_cast<uint16_t>(vertices.size());
			vertices.push_back(vertex);
		}

		indices.push_back(uniqueVertices.at(vertex));
	}
}

void Mesh::draw(const vk::CommandBuffer &commandBuffer) const
{
	bind(commandBuffer);
	commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void Mesh::bind(const vk::CommandBuffer &commandBuffer) const
{
	std::array<vk::DeviceSize, 1> offsets{0};
	std::array<vk::Buffer, 1> vertexBuffers{vertexBuffer};
	commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
	commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);
}

void Mesh::destroy(const VmaAllocator &vmaAllocator)
{
	vmaDestroyBuffer(vmaAllocator, vertexBuffer, vertexBufferAllocation);
	vmaDestroyBuffer(vmaAllocator, indexBuffer, indexBufferAllocation);
}

void Mesh::uploadMesh(const VmaAllocator &vmaAllocator, const vk::Queue &queue, const vk::CommandBuffer &commandBuffer)
{
	VkDeviceSize verticesSize = vertices.size() * sizeof(Vertex);
	VkDeviceSize indicesSize = indices.size() * sizeof(uint16_t);

	VkDeviceSize stagingBufferSize = std::max(verticesSize, indicesSize);

	VkBufferCreateInfo stagingBufferCreateInfo{};
	stagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBufferCreateInfo.size = stagingBufferSize;

	VmaAllocationCreateInfo vmaStagingBufferAllocCreateInfo{};
	vmaStagingBufferAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
	vmaStagingBufferAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;
	VmaAllocationInfo stagingBufferAllocInfo;
	vmaCreateBuffer(vmaAllocator, &stagingBufferCreateInfo, &vmaStagingBufferAllocCreateInfo, &stagingBuffer, &stagingBufferAllocation, &stagingBufferAllocInfo);

	VkBufferCreateInfo vertexBufferCreateInfo{};
	vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vertexBufferCreateInfo.size = verticesSize;

	VmaAllocationCreateInfo vmaAllocCreateInfo{};
	vmaAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	vmaCreateBuffer(vmaAllocator, &vertexBufferCreateInfo, &vmaAllocCreateInfo, &vertexBuffer, &vertexBufferAllocation, nullptr);

	VkBufferCreateInfo indexBufferCreateInfo{};
	indexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	indexBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	indexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	indexBufferCreateInfo.size = indicesSize;

	vmaCreateBuffer(vmaAllocator, &indexBufferCreateInfo, &vmaAllocCreateInfo, &indexBuffer, &indexBufferAllocation, nullptr);

	memcpy(stagingBufferAllocInfo.pMappedData, vertices.data(), verticesSize);

	CommandBuffer::beginSTC(commandBuffer);
	vk::BufferCopy bufferCopy;
	bufferCopy.setSize(verticesSize);
	commandBuffer.copyBuffer(stagingBuffer, vertexBuffer, bufferCopy);
	CommandBuffer::endSTC(commandBuffer, queue);

	memcpy(stagingBufferAllocInfo.pMappedData, indices.data(), indicesSize);

	CommandBuffer::beginSTC(commandBuffer);
	bufferCopy.setSize(indicesSize);
	commandBuffer.copyBuffer(stagingBuffer, indexBuffer, bufferCopy);
	CommandBuffer::endSTC(commandBuffer, queue);

	vmaDestroyBuffer(vmaAllocator, stagingBuffer, stagingBufferAllocation);
}

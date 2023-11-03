#include "Model.h"

Model::Model(const VmaAllocator &vmaAllocator, const vk::Queue &queue, const vk::CommandBuffer &commandBuffer, const char *path)
{
	tinyobj::ObjReaderConfig objReaderConfig;
	objReaderConfig.triangulate = true;

	tinyobj::ObjReader objReader;
	objReader.ParseFromFile(path, objReaderConfig);

	for (const auto &shape : objReader.GetShapes())
	{
		meshes.emplace_back(shape, objReader.GetAttrib());
	}

	uploadMeshes(vmaAllocator, queue, commandBuffer);
}

void Model::draw(const vk::CommandBuffer &commandBuffer) const
{
	for (const auto &mesh : meshes)
	{
		mesh.draw(commandBuffer);
	}
}

void Model::destroy(const VmaAllocator &vmaAllocator)
{
	for (auto &mesh : meshes)
	{
		mesh.destroy(vmaAllocator);
	}
}

void Model::uploadMeshes(const VmaAllocator &vmaAllocator, const vk::Queue &queue, const vk::CommandBuffer &commandBuffer)
{
	for (auto &mesh : meshes)
	{
		mesh.uploadMesh(vmaAllocator, queue, commandBuffer);
	}
}

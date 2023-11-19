#include "Model.h"

namespace N
{
Model::Model(const ModelCreateInfo &createInfo, const char *path)
{
	tinyobj::ObjReaderConfig objReaderConfig;
	objReaderConfig.triangulate = true;

	tinyobj::ObjReader objReader;
	objReader.ParseFromFile(path, objReaderConfig);

	for (const auto &material : objReader.GetMaterials())
	{
		Material cur(createInfo.vmaAllocator, createInfo.device, createInfo.queue, createInfo.commandBuffer, material);
		cur.createSampler(createInfo.device, createInfo.maxAnisotropy);
		cur.createDescriptorSets(createInfo.device, createInfo.descriptorPool, createInfo.descriptorSetLayout);
		materials.push_back(std::move(cur));
	}

	for (const auto &shape : objReader.GetShapes())
	{
		meshes.emplace_back(shape, objReader.GetAttrib(), shape.mesh.material_ids.at(0));
	}

	uploadMeshes(createInfo);
}

void Model::draw(const vk::CommandBuffer &commandBuffer, const vk::PipelineLayout &pipelineLayout) const
{
	for (const auto &mesh : meshes)
	{
		materials.at(mesh.getMaterialId()).bind(commandBuffer, pipelineLayout);
		mesh.draw(commandBuffer);
	}
}

void Model::destroy(const VmaAllocator &vmaAllocator, const vk::Device &device)
{
	for (auto &mat : materials)
	{
		mat.destroy(vmaAllocator, device);
	}

	for (auto &mesh : meshes)
	{
		mesh.destroy(vmaAllocator);
	}
}

void Model::uploadMeshes(const ModelCreateInfo &createInfo)
{
	for (auto &mesh : meshes)
	{
		mesh.uploadMesh(createInfo.vmaAllocator, createInfo.queue, createInfo.commandBuffer);
	}
}
} // namespace N

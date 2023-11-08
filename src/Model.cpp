#include "Model.h"

#include "Material.h"

Model::Model(const VmaAllocator &vmaAllocator, const vk::Device &device, const vk::Queue &queue,
			 const vk::CommandBuffer &commandBuffer, const vk::DescriptorPool &descriptorPool,
			 const vk::DescriptorSetLayout &descriptorSetLayout, const vk::Sampler &sampler, const char *path)
{
	tinyobj::ObjReaderConfig objReaderConfig;
	objReaderConfig.triangulate = true;

	tinyobj::ObjReader objReader;
	objReader.ParseFromFile(path, objReaderConfig);

	for (const auto &material : objReader.GetMaterials())
	{
		Material cur(vmaAllocator, device, queue, commandBuffer, material);
		materials.push_back(std::move(cur));
		materials.at(materials.size() - 1).createDescriptorSets(device, descriptorPool, descriptorSetLayout, sampler);
	}

	for (const auto &shape : objReader.GetShapes())
	{
		meshes.emplace_back(shape, objReader.GetAttrib(), shape.mesh.material_ids.at(0));
	}

	uploadMeshes(vmaAllocator, queue, commandBuffer);
}

void Model::draw(const vk::CommandBuffer &commandBuffer, const vk::PipelineLayout &pipelineLayout) const
{
	for (const auto &mesh : meshes)
	{
		materials.at(mesh.getMaterialId()).bind(commandBuffer, pipelineLayout);
		mesh.draw(commandBuffer);
	}
}

void Model::destroy(const VmaAllocator &vmaAllocator, const vk::Device &device,
					const vk::DescriptorPool &descriptorPool)
{
	for (auto &mat : materials)
	{
		mat.destroy(vmaAllocator, device, descriptorPool);
	}

	for (auto &mesh : meshes)
	{
		mesh.destroy(vmaAllocator);
	}
}

void Model::uploadMeshes(const VmaAllocator &vmaAllocator, const vk::Queue &queue,
						 const vk::CommandBuffer &commandBuffer)
{
	for (auto &mesh : meshes)
	{
		mesh.uploadMesh(vmaAllocator, queue, commandBuffer);
	}
}

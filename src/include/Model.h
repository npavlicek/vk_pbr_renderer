#pragma once

#include <iostream>

#include <vma/vk_mem_alloc.h>

#include "Mesh.h"

class Model
{
public:
	Model() = delete;
	Model(const VmaAllocator &vmaAllocator, const vk::Queue &queue, const vk::CommandBuffer &commandBuffer, const char *path);
	Model(const Model &rhs) = delete;

	const std::vector<Mesh> &getMeshes()
	{
		return meshes;
	}

	void draw(const vk::CommandBuffer &commandBuffer) const;
	void destroy(const VmaAllocator &vmaAllocator);

private:
	std::vector<Mesh> meshes;

	void uploadMeshes(const VmaAllocator &vmaAllocator, const vk::Queue &queue, const vk::CommandBuffer &commandBuffer);
};

#pragma once

#include <iostream>

#include <vma/vk_mem_alloc.h>

#include "Mesh.h"

class Model
{
public:
	Model() = delete;
	Model(const VmaAllocator &vmaAllocator, const vk::Queue &queue, const vk::CommandBuffer &commandBuffer, const char *path);
	Model(const Model &) = delete;
	Model &operator=(const Model &) = delete;
	constexpr Model(Model &&) = default;

	const std::vector<Mesh> &getMeshes() const
	{
		return meshes;
	}

	const glm::mat4 &getModel() const
	{
		return model;
	}

	void draw(const vk::CommandBuffer &commandBuffer) const;
	void destroy(const VmaAllocator &vmaAllocator);

private:
	std::vector<Mesh> meshes;
	glm::mat4 model{1.f};

	void uploadMeshes(const VmaAllocator &vmaAllocator, const vk::Queue &queue, const vk::CommandBuffer &commandBuffer);
};

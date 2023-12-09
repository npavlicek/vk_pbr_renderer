#pragma once

#include <cstdint>
#include <fstream>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include <glm/glm.hpp>

namespace N
{
struct SkyboxPipelineCreateInfo
{
	vk::Device device;
	vk::SampleCountFlagBits samples;
	vk::RenderPass renderPass;
	std::vector<uint32_t> vertexShaderCode;
	std::vector<uint32_t> fragmentShaderCode;
	vk::PushConstantRange pushConstant;
};

class SkyboxPipeline
{
  public:
	constexpr SkyboxPipeline() = default;
	constexpr SkyboxPipeline(SkyboxPipeline &rhs) = delete;
	constexpr SkyboxPipeline &operator=(SkyboxPipeline &rhs) = delete;
	constexpr SkyboxPipeline(SkyboxPipeline &&rhs) = default;
	constexpr SkyboxPipeline &operator=(SkyboxPipeline &&rhs) = default;

	void create(const SkyboxPipelineCreateInfo &createInfo);
	void destroy(const vk::Device &device);

	const vk::Pipeline &getPipeline()
	{
		return pipeline;
	}

	const vk::PipelineLayout &getPipelineLayout()
	{
		return pipelineLayout;
	}

	const vk::DescriptorSetLayout &getCubeMapLayout()
	{
		return cubeMapLayout;
	}

  private:
	vk::Pipeline pipeline;
	vk::PipelineLayout pipelineLayout;
	vk::DescriptorSetLayout cubeMapLayout;

	vk::ShaderModule vertexShader;
	vk::ShaderModule fragmentShader;

	void createShaderModules(const SkyboxPipelineCreateInfo &createInfo);
	void destroyShaderModules(const vk::Device &device);
};
} // namespace N

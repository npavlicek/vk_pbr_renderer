#pragma once

#include <fstream>
#include <optional>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include <glm/glm.hpp>

namespace N
{
struct PBRPipelineCreateInfo
{
	vk::Device device;
	vk::SampleCountFlagBits samples;
	vk::RenderPass renderPass;
	std::vector<uint32_t> vertexShaderCode;
	std::vector<uint32_t> fragmentShaderCode;
	vk::PushConstantRange pushConstant;
};

class PBRPipeline
{
  public:
	constexpr PBRPipeline() = default;
	constexpr PBRPipeline(PBRPipeline &rhs) = delete;
	constexpr PBRPipeline &operator=(PBRPipeline &rhs) = delete;
	constexpr PBRPipeline(PBRPipeline &&rhs) = default;
	constexpr PBRPipeline &operator=(PBRPipeline &&rhs) = default;

	void create(const PBRPipelineCreateInfo &createInfo);
	void destroy(const vk::Device &device);

	const vk::Pipeline &getPipeline()
	{
		return pipeline;
	}

	const vk::PipelineLayout &getPipelineLayout()
	{
		return pipelineLayout;
	}

	const vk::DescriptorSetLayout &getTextureSetLayout()
	{
		return textureSetLayout;
	}

	const vk::DescriptorSetLayout &getRenderInfoLayout()
	{
		return renderInfoLayout;
	}

  private:
	vk::Pipeline pipeline;
	vk::PipelineLayout pipelineLayout;
	vk::DescriptorSetLayout textureSetLayout;
	vk::DescriptorSetLayout renderInfoLayout;

	vk::ShaderModule vertexShader;
	vk::ShaderModule fragmentShader;

	void createShaderModules(const PBRPipelineCreateInfo &createInfo);
	void destroyShaderModules(const vk::Device &device);
};
} // namespace N

#pragma once

#include <fstream>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>

#include <glm/glm.hpp>

namespace N
{
struct PBRPipelineCreateInfo
{
	vk::Device device;
	vk::SampleCountFlagBits samples;
	vk::RenderPass renderPass;
};

struct MVPPushConstant
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

class PBRPipeline
{
  public:
	constexpr PBRPipeline(PBRPipeline &rhs) = delete;
	constexpr PBRPipeline &operator=(PBRPipeline &rhs) = delete;
	constexpr PBRPipeline(PBRPipeline &&rhs) = default;
	PBRPipeline &operator=(PBRPipeline &&rhs) = default;

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

	void createShaderModules(const vk::Device &device);
	void destroyShaderModules(const vk::Device &device);
	std::vector<uint32_t> loadShaderCode(const char *path);
};
} // namespace N

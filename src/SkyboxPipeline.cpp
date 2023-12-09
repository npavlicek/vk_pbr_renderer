#include "SkyboxPipeline.h"

#include "VkErrorHandling.h"
#include "Vertex.h"
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace N
{
void SkyboxPipeline::create(const SkyboxPipelineCreateInfo &createInfo)
{
	createShaderModules(createInfo);

	// Shader Stages
	vk::PipelineShaderStageCreateInfo vertexShaderStageCreateInfo;
	vertexShaderStageCreateInfo.setModule(vertexShader);
	vertexShaderStageCreateInfo.setPName("main");
	vertexShaderStageCreateInfo.setStage(vk::ShaderStageFlagBits::eVertex);

	vk::PipelineShaderStageCreateInfo fragmentShaderStageCreateInfo;
	fragmentShaderStageCreateInfo.setModule(fragmentShader);
	fragmentShaderStageCreateInfo.setPName("main");
	fragmentShaderStageCreateInfo.setStage(vk::ShaderStageFlagBits::eFragment);

	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages{vertexShaderStageCreateInfo,
																fragmentShaderStageCreateInfo};

	// Fixed Functions

	// Dynamic State
	std::vector<vk::DynamicState> dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};

	vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo;
	pipelineDynamicStateCreateInfo.setDynamicStateCount(dynamicStates.size());
	pipelineDynamicStateCreateInfo.setDynamicStates(dynamicStates);

	// Vertex Input
	auto vertexAttributeDescription = Vertex::getAttributeDescription();
	auto vertexBindingDescription = Vertex::getBindingDescription();

	vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
	pipelineVertexInputStateCreateInfo.setVertexAttributeDescriptionCount(vertexAttributeDescription.size());
	pipelineVertexInputStateCreateInfo.setVertexAttributeDescriptions(vertexAttributeDescription);
	pipelineVertexInputStateCreateInfo.setVertexBindingDescriptionCount(1);
	pipelineVertexInputStateCreateInfo.setVertexBindingDescriptions(vertexBindingDescription);

	// Input Assembly
	vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo;
	pipelineInputAssemblyStateCreateInfo.setPrimitiveRestartEnable(vk::False);
	pipelineInputAssemblyStateCreateInfo.setTopology(vk::PrimitiveTopology::eTriangleList);

	// Viewport
	vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo;
	pipelineViewportStateCreateInfo.setScissorCount(1);
	pipelineViewportStateCreateInfo.setViewportCount(1);

	// Rasterization
	vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo;
	pipelineRasterizationStateCreateInfo.setDepthClampEnable(vk::False);
	pipelineRasterizationStateCreateInfo.setRasterizerDiscardEnable(vk::False);
	pipelineRasterizationStateCreateInfo.setPolygonMode(vk::PolygonMode::eFill);
	pipelineRasterizationStateCreateInfo.setLineWidth(1.f);
	pipelineRasterizationStateCreateInfo.setCullMode(vk::CullModeFlagBits::eBack);
	pipelineRasterizationStateCreateInfo.setFrontFace(vk::FrontFace::eCounterClockwise);
	pipelineRasterizationStateCreateInfo.setDepthBiasEnable(vk::False);

	// Multisampling
	vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
	pipelineMultisampleStateCreateInfo.setRasterizationSamples(createInfo.samples);
	pipelineMultisampleStateCreateInfo.setSampleShadingEnable(vk::True);
	pipelineMultisampleStateCreateInfo.setMinSampleShading(.2f);

	// Depth testing
	vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
	pipelineDepthStencilStateCreateInfo.setDepthWriteEnable(vk::True);
	pipelineDepthStencilStateCreateInfo.setDepthTestEnable(vk::True);
	pipelineDepthStencilStateCreateInfo.setDepthCompareOp(vk::CompareOp::eLess);
	pipelineDepthStencilStateCreateInfo.setDepthBoundsTestEnable(vk::False);
	pipelineDepthStencilStateCreateInfo.setStencilTestEnable(vk::False);
	pipelineDepthStencilStateCreateInfo.setMinDepthBounds(0.f);
	pipelineDepthStencilStateCreateInfo.setMaxDepthBounds(1.f);

	// Color blending
	vk::PipelineColorBlendAttachmentState colorBlendAttachmentState;
	colorBlendAttachmentState.setBlendEnable(vk::False);
	colorBlendAttachmentState.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
												vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

	vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
	pipelineColorBlendStateCreateInfo.setAttachmentCount(1);
	pipelineColorBlendStateCreateInfo.setAttachments(colorBlendAttachmentState);
	pipelineColorBlendStateCreateInfo.setLogicOpEnable(vk::False);

	// Pipeline Layout	
	vk::DescriptorSetLayoutBinding cubeMapBinding{};
	cubeMapBinding.setBinding(0);
	cubeMapBinding.setDescriptorCount(1);
	cubeMapBinding.setStageFlags(vk::ShaderStageFlagBits::eFragment);
	cubeMapBinding.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);

	vk::DescriptorSetLayoutCreateInfo cubeMapLayoutCreateInfo{};
	cubeMapLayoutCreateInfo.setBindingCount(1);
	cubeMapLayoutCreateInfo.setBindings(cubeMapBinding);

	cubeMapLayout = createInfo.device.createDescriptorSetLayout(cubeMapLayoutCreateInfo);

	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.setSetLayoutCount(1);
	pipelineLayoutCreateInfo.setSetLayouts(cubeMapLayout);
	pipelineLayoutCreateInfo.setPushConstantRangeCount(1);
	pipelineLayoutCreateInfo.setPushConstantRanges(createInfo.pushConstant);

	pipelineLayout = createInfo.device.createPipelineLayout(pipelineLayoutCreateInfo);

	vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
	graphicsPipelineCreateInfo.setLayout(pipelineLayout);
	graphicsPipelineCreateInfo.setRenderPass(createInfo.renderPass);
	graphicsPipelineCreateInfo.setStageCount(shaderStages.size());
	graphicsPipelineCreateInfo.setStages(shaderStages);
	graphicsPipelineCreateInfo.setPColorBlendState(&pipelineColorBlendStateCreateInfo);
	graphicsPipelineCreateInfo.setPDynamicState(&pipelineDynamicStateCreateInfo);
	graphicsPipelineCreateInfo.setPInputAssemblyState(&pipelineInputAssemblyStateCreateInfo);
	graphicsPipelineCreateInfo.setPMultisampleState(&pipelineMultisampleStateCreateInfo);
	graphicsPipelineCreateInfo.setPRasterizationState(&pipelineRasterizationStateCreateInfo);
	graphicsPipelineCreateInfo.setPVertexInputState(&pipelineVertexInputStateCreateInfo);
	graphicsPipelineCreateInfo.setPViewportState(&pipelineViewportStateCreateInfo);
	graphicsPipelineCreateInfo.setPDepthStencilState(&pipelineDepthStencilStateCreateInfo);

	// TODO: implement pipeline caching?

	auto pipelineResult = createInfo.device.createGraphicsPipeline(nullptr, graphicsPipelineCreateInfo);
	checkResult(pipelineResult.result);

	pipeline = pipelineResult.value;

	destroyShaderModules(createInfo.device);
}

void SkyboxPipeline::destroy(const vk::Device &device)
{
	device.destroyPipelineLayout(pipelineLayout);
	device.destroyPipeline(pipeline);
}

void SkyboxPipeline::createShaderModules(const SkyboxPipelineCreateInfo &createInfo)
{
	vk::ShaderModuleCreateInfo vertexShaderModuleCreateInfo;
	vertexShaderModuleCreateInfo.setCode(createInfo.vertexShaderCode);
	vertexShaderModuleCreateInfo.setCodeSize(createInfo.vertexShaderCode.size() * sizeof(uint32_t));

	vk::ShaderModuleCreateInfo fragmentShaderModuleCreateInfo;
	fragmentShaderModuleCreateInfo.setCode(createInfo.fragmentShaderCode);
	fragmentShaderModuleCreateInfo.setCodeSize(createInfo.fragmentShaderCode.size() * sizeof(uint32_t));

	vertexShader = createInfo.device.createShaderModule(vertexShaderModuleCreateInfo);
	fragmentShader = createInfo.device.createShaderModule(fragmentShaderModuleCreateInfo);
}

void SkyboxPipeline::destroyShaderModules(const vk::Device &device)
{
	device.destroyShaderModule(vertexShader);
	device.destroyShaderModule(fragmentShader);
}
} // namespace N

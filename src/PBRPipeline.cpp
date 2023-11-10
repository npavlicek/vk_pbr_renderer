#include "PBRPipeline.h"

#include "VkErrorHandling.h"
#include "Vertex.h"

namespace N
{
void PBRPipeline::create(const PBRPipelineCreateInfo &createInfo)
{
	createShaderModules(createInfo.device);

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
	pipelineRasterizationStateCreateInfo.setCullMode(vk::CullModeFlags{vk::CullModeFlagBits::eBack});
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

	vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo;
	pipelineColorBlendStateCreateInfo.setAttachmentCount(1);
	pipelineColorBlendStateCreateInfo.setAttachments(colorBlendAttachmentState);
	pipelineColorBlendStateCreateInfo.setLogicOpEnable(vk::False);

	// Push Constants
	vk::PushConstantRange pushConstants;
	pushConstants.setOffset(0);
	pushConstants.setSize(sizeof(MVPPushConstant));
	pushConstants.setStageFlags(vk::ShaderStageFlagBits::eVertex);

	// Descriptor Set Layouts
	vk::DescriptorSetLayoutBinding diffuseSamplerBinding{};
	diffuseSamplerBinding.setBinding(0);
	diffuseSamplerBinding.setDescriptorCount(1);
	diffuseSamplerBinding.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	diffuseSamplerBinding.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorSetLayoutBinding metallicSamplerBinding{};
	metallicSamplerBinding.setBinding(1);
	metallicSamplerBinding.setDescriptorCount(1);
	metallicSamplerBinding.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	metallicSamplerBinding.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorSetLayoutBinding roughnessSamplerBinding{};
	roughnessSamplerBinding.setBinding(2);
	roughnessSamplerBinding.setDescriptorCount(1);
	roughnessSamplerBinding.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	roughnessSamplerBinding.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorSetLayoutBinding normalSamplerBinding{};
	normalSamplerBinding.setBinding(3);
	normalSamplerBinding.setDescriptorCount(1);
	normalSamplerBinding.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	normalSamplerBinding.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	std::array<vk::DescriptorSetLayoutBinding, 4> textureBindings{diffuseSamplerBinding, metallicSamplerBinding,
																  roughnessSamplerBinding, normalSamplerBinding};

	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
	descriptorSetLayoutCI.setBindingCount(textureBindings.size());
	descriptorSetLayoutCI.setBindings(textureBindings);

	textureSetLayout = createInfo.device.createDescriptorSetLayout(descriptorSetLayoutCI);

	vk::DescriptorSetLayoutBinding renderInfoBinding{};
	renderInfoBinding.setBinding(4);
	renderInfoBinding.setDescriptorCount(1);
	renderInfoBinding.setStageFlags(vk::ShaderStageFlagBits::eFragment);
	renderInfoBinding.setDescriptorType(vk::DescriptorType::eUniformBuffer);

	descriptorSetLayoutCI.setBindingCount(1);
	descriptorSetLayoutCI.setBindings(renderInfoBinding);

	renderInfoLayout = createInfo.device.createDescriptorSetLayout(descriptorSetLayoutCI);

	std::array<vk::DescriptorSetLayout, 2> descriptorLayouts{renderInfoLayout, textureSetLayout};

	// Pipeline Layout
	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	pipelineLayoutCreateInfo.setSetLayoutCount(descriptorLayouts.size());
	pipelineLayoutCreateInfo.setSetLayouts(descriptorLayouts);
	pipelineLayoutCreateInfo.setPushConstantRanges(pushConstants);
	pipelineLayoutCreateInfo.setPushConstantRangeCount(1);

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

void PBRPipeline::destroy(const vk::Device &device)
{
	device.destroyDescriptorSetLayout(textureSetLayout);
	device.destroyDescriptorSetLayout(renderInfoLayout);
	device.destroyPipelineLayout(pipelineLayout);
	device.destroyPipeline(pipeline);
}

void PBRPipeline::createShaderModules(const vk::Device &device)
{
	auto vertexShaderCode = loadShaderCode("shaders/vert.spv");
	auto fragmentShaderCode = loadShaderCode("shaders/frag.spv");

	vk::ShaderModuleCreateInfo vertexShaderModuleCreateInfo;
	vertexShaderModuleCreateInfo.setCode(vertexShaderCode);
	vertexShaderModuleCreateInfo.setCodeSize(vertexShaderCode.size() * sizeof(uint32_t));

	vk::ShaderModuleCreateInfo fragmentShaderModuleCreateInfo;
	fragmentShaderModuleCreateInfo.setCode(fragmentShaderCode);
	fragmentShaderModuleCreateInfo.setCodeSize(fragmentShaderCode.size() * sizeof(uint32_t));

	vertexShader = device.createShaderModule(vertexShaderModuleCreateInfo);
	fragmentShader = device.createShaderModule(fragmentShaderModuleCreateInfo);
}

void PBRPipeline::destroyShaderModules(const vk::Device &device)
{
	device.destroyShaderModule(vertexShader);
	device.destroyShaderModule(fragmentShader);
}

std::vector<uint32_t> PBRPipeline::loadShaderCode(const char *path)
{
	std::ifstream shaderFile(path, std::ios::in | std::ios::binary);

	std::streamsize size;

	shaderFile.seekg(0, std::ios::end);
	size = shaderFile.tellg();
	shaderFile.seekg(0, std::ios::beg);

	std::vector<uint32_t> res;

	if (shaderFile.is_open())
	{
		uint32_t current;
		while (shaderFile.tellg() < size)
		{
			shaderFile.read(reinterpret_cast<char *>(&current), sizeof(current));
			res.push_back(current);
		}
	}
	else
	{
		std::cerr << "Could not open shader file!" << std::endl;
		throw std::runtime_error(path);
	}

	shaderFile.close();

	return res;
}
} // namespace N

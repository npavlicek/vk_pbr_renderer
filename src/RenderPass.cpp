#include "RenderPass.h"
#include <vulkan/vulkan_structs.hpp>

namespace N
{
void RenderPass::create(const RenderPassCreateInfo &createInfo)
{
	// multisamples color
	vk::AttachmentDescription attachmentDescription;
	attachmentDescription.setLoadOp(vk::AttachmentLoadOp::eClear);
	attachmentDescription.setStoreOp(vk::AttachmentStoreOp::eStore);
	attachmentDescription.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	attachmentDescription.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
	attachmentDescription.setInitialLayout(vk::ImageLayout::eUndefined);
	attachmentDescription.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
	attachmentDescription.setSamples(createInfo.samples);
	attachmentDescription.setFormat(createInfo.surfaceFormat);

	// depth
	vk::AttachmentDescription depthAttachmentDescription{};
	depthAttachmentDescription.setFormat(createInfo.depthFormat);
	depthAttachmentDescription.setInitialLayout(vk::ImageLayout::eUndefined);
	depthAttachmentDescription.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	depthAttachmentDescription.setSamples(createInfo.samples);
	depthAttachmentDescription.setStoreOp(vk::AttachmentStoreOp::eDontCare);
	depthAttachmentDescription.setLoadOp(vk::AttachmentLoadOp::eClear);
	depthAttachmentDescription.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
	depthAttachmentDescription.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

	// resolved color 1 sample
	vk::AttachmentDescription colorResolve{};
	colorResolve.setFormat(createInfo.surfaceFormat);
	colorResolve.setSamples(vk::SampleCountFlagBits::e1);
	colorResolve.setLoadOp(vk::AttachmentLoadOp::eDontCare);
	colorResolve.setStoreOp(vk::AttachmentStoreOp::eStore);
	colorResolve.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
	colorResolve.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	colorResolve.setInitialLayout(vk::ImageLayout::eUndefined);
	colorResolve.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	vk::AttachmentReference colorAttachmentRef{};
	colorAttachmentRef.setAttachment(0);
	colorAttachmentRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::AttachmentReference depthAttachmentReference{};
	depthAttachmentReference.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	depthAttachmentReference.setAttachment(1);

	vk::AttachmentReference resolveRef{};
	resolveRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	resolveRef.setAttachment(2);

	vk::SubpassDescription subpassDescription;
	subpassDescription.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	subpassDescription.setColorAttachmentCount(1);
	subpassDescription.setColorAttachments(colorAttachmentRef);
	subpassDescription.setPDepthStencilAttachment(&depthAttachmentReference);
	subpassDescription.setResolveAttachments(resolveRef);

	std::array<vk::SubpassDependency, 2> subpassDependencies;

	subpassDependencies.at(0).setSrcSubpass(vk::SubpassExternal);
	subpassDependencies.at(0).setDstSubpass(0);
	subpassDependencies.at(0).setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite);
	subpassDependencies.at(0).setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite |
											   vk::AccessFlagBits::eDepthStencilAttachmentRead);
	subpassDependencies.at(0).setSrcStageMask(vk::PipelineStageFlagBits::eLateFragmentTests);
	subpassDependencies.at(0).setDstStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests);

	subpassDependencies.at(1).setSrcSubpass(vk::SubpassExternal);
	subpassDependencies.at(1).setDstSubpass(0);
	subpassDependencies.at(1).setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
	subpassDependencies.at(1).setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead |
											   vk::AccessFlagBits::eColorAttachmentWrite);
	subpassDependencies.at(1).setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
	subpassDependencies.at(1).setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);

	std::vector<vk::AttachmentDescription> attachments{attachmentDescription, depthAttachmentDescription, colorResolve};

	vk::RenderPassCreateInfo renderPassCreateInfo;
	renderPassCreateInfo.setAttachmentCount(attachments.size());
	renderPassCreateInfo.setAttachments(attachments);
	renderPassCreateInfo.setSubpassCount(1);
	renderPassCreateInfo.setSubpasses(subpassDescription);
	renderPassCreateInfo.setDependencies(subpassDependencies);
	renderPassCreateInfo.setDependencyCount(subpassDependencies.size());

	renderPass = createInfo.device.createRenderPass(renderPassCreateInfo);
}

void RenderPass::destroy(const vk::Device &device)
{
	device.destroyRenderPass(renderPass);
}
} // namespace N

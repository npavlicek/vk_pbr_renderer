#include "Texture.h"

#include "stb_image.h"

#include "CommandBuffer.h"
#include "Util.h"


#include <string>

Texture::Texture(vk::raii::Device &device, vk::raii::PhysicalDevice &physicalDevice,
				 vk::raii::CommandBuffer &commandBuffer, vk::raii::Queue &queue, const std::string &path)
{
	width = 0;
	height = 0;
	channels = 0;
	unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

	if (!data)
		throw std::runtime_error("Error loading image: " + path);

	imageSize = width * height * STBI_rgb_alpha;

	auto imageRes = util::createImage(
		device, physicalDevice, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::Format::eR8G8B8A8Srgb,
		vk::Extent3D{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1}, vk::ImageType::e2D,
		vk::MemoryPropertyFlagBits::eDeviceLocal, vk::SampleCountFlagBits::e1);

	image = std::make_unique<vk::raii::Image>(std::move(std::get<vk::raii::Image>(imageRes)));
	imageMemory = std::make_unique<vk::raii::DeviceMemory>(std::move(std::get<vk::raii::DeviceMemory>(imageRes)));

	const auto physicalDeviceMemProps = physicalDevice.getMemoryProperties();

	auto bufferRes =
		util::createBuffer(device, physicalDeviceMemProps,
						   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
						   imageSize, vk::BufferUsageFlagBits::eTransferSrc);

	const auto stagingBuffer = std::make_unique<vk::raii::Buffer>(std::move(std::get<vk::raii::Buffer>(bufferRes)));
	const auto stagingBufferMemory =
		std::make_unique<vk::raii::DeviceMemory>(std::move(std::get<vk::raii::DeviceMemory>(bufferRes)));

	void *mappedMemory = stagingBufferMemory->mapMemory(0, imageSize, vk::MemoryMapFlags{});
	memcpy(mappedMemory, data, imageSize);
	stagingBufferMemory->unmapMemory();

	stbi_image_free(data);

	CommandBuffer::beginSTC(*commandBuffer);
	transitionImage(commandBuffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
	copyBufferToImage(commandBuffer, *stagingBuffer);
	transitionImage(commandBuffer, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
	CommandBuffer::endSTC(*commandBuffer, *queue);

	imageView = std::make_unique<vk::raii::ImageView>(
		util::createImageView(device, **image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor));

	const auto physicalDeviceProperties = physicalDevice.getProperties();

	createSampler(device, physicalDeviceProperties.limits.maxSamplerAnisotropy);
}

void Texture::createSampler(vk::raii::Device &device, float maxAnisotropy)
{
	vk::SamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.setMagFilter(vk::Filter::eLinear);
	samplerCreateInfo.setMinFilter(vk::Filter::eLinear);
	samplerCreateInfo.setAddressModeU(vk::SamplerAddressMode::eRepeat);
	samplerCreateInfo.setAddressModeV(vk::SamplerAddressMode::eRepeat);
	samplerCreateInfo.setAddressModeW(vk::SamplerAddressMode::eRepeat);
	samplerCreateInfo.setAnisotropyEnable(vk::True);
	samplerCreateInfo.setMaxAnisotropy(maxAnisotropy);
	samplerCreateInfo.setBorderColor(vk::BorderColor::eFloatOpaqueWhite);
	samplerCreateInfo.setUnnormalizedCoordinates(vk::False);
	samplerCreateInfo.setCompareEnable(vk::False);
	samplerCreateInfo.setCompareOp(vk::CompareOp::eAlways);
	samplerCreateInfo.setMipmapMode(vk::SamplerMipmapMode::eLinear);
	samplerCreateInfo.setMipLodBias(0.f);
	samplerCreateInfo.setMinLod(0.f);
	samplerCreateInfo.setMaxLod(0.f);
	imageSampler = std::make_unique<vk::raii::Sampler>(device.createSampler(samplerCreateInfo));
}

void Texture::copyBufferToImage(vk::raii::CommandBuffer &buffer, vk::raii::Buffer &stagingBuffer)
{
	vk::BufferImageCopy bufferImageCopy{};
	bufferImageCopy.setImageExtent(vk::Extent3D{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1});
	bufferImageCopy.setImageSubresource(vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1});

	buffer.copyBufferToImage(*stagingBuffer, **image, vk::ImageLayout::eTransferDstOptimal, bufferImageCopy);
}

void Texture::transitionImage(vk::raii::CommandBuffer &buffer, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
	vk::ImageMemoryBarrier imageMemoryBarrier{};
	imageMemoryBarrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
	imageMemoryBarrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
	imageMemoryBarrier.setImage(**image);
	imageMemoryBarrier.setOldLayout(oldLayout);
	imageMemoryBarrier.setNewLayout(newLayout);
	imageMemoryBarrier.setSubresourceRange(vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

	vk::PipelineStageFlags src;
	vk::PipelineStageFlags dst;

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eNone);
		imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

		src = vk::PipelineStageFlagBits::eTopOfPipe;
		dst = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
		imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

		src = vk::PipelineStageFlagBits::eTransfer;
		dst = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else
	{
		throw std::invalid_argument("Image layouts for image layout transition not supported!");
	}

	buffer.pipelineBarrier(src, dst, vk::DependencyFlagBits::eByRegion, nullptr, nullptr, imageMemoryBarrier);
}

#include "Util.h"

namespace util
{
	vk::raii::Instance createInstance(
		vk::raii::Context &context,
		const char *applicationName,
		const char *engineName,
		std::optional<vk::DebugUtilsMessengerCreateInfoEXT> debugUtilsMessengerCreateInfo)
	{
		std::vector<const char *> enabledLayers{};
		std::vector<const char *> enabledExtensions{};

#ifdef ENABLE_VULKAN_VALIDATION_LAYERS
		enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
		enabledLayers.push_back("VK_LAYER_LUNARG_monitor");
		Validation::areLayersAvailable(context, enabledLayers);
#endif

		// Add GLFW necessary extensions
		uint32_t glfwExtensionCount = 0;
		const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		enabledExtensions.reserve(glfwExtensionCount);
		for (int i = 0; i < static_cast<int>(glfwExtensionCount); i++)
		{
			enabledExtensions.push_back(glfwExtensions[i]);
		}

		if (debugUtilsMessengerCreateInfo.has_value())
		{
			enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		vk::ApplicationInfo applicationInfo(
			applicationName,
			1,
			engineName,
			1,
			vk::ApiVersion13);

		vk::InstanceCreateInfo instanceCreateInfo{};
		instanceCreateInfo.setPNext(&debugUtilsMessengerCreateInfo.value());
		instanceCreateInfo.setPApplicationInfo(&applicationInfo);
		instanceCreateInfo.setEnabledLayerCount(enabledLayers.size());
		instanceCreateInfo.setPEnabledLayerNames(enabledLayers);
		instanceCreateInfo.setEnabledExtensionCount(enabledExtensions.size());
		instanceCreateInfo.setPEnabledExtensionNames(enabledExtensions);

		vk::raii::Instance instance(
			context,
			instanceCreateInfo);

		return instance;
	}

	vk::raii::PhysicalDevice selectPhysicalDevice(
		vk::raii::Instance &instance)
	{
		auto physicalDevices = instance.enumeratePhysicalDevices();

		vk::raii::PhysicalDevice selectedDevice(nullptr);
		for (const auto &pDevice : physicalDevices)
		{
			if (pDevice.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			{
				selectedDevice = pDevice;
				break;
			}
		}

		std::cout << "Selected Device: " << selectedDevice.getProperties().deviceName << std::endl;

		return selectedDevice;
	}

	int selectQueueFamily(
		vk::raii::PhysicalDevice &physicalDevice)
	{
		auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

		int selectedQueueFamilyIndex = 0;
		for (const auto &queueFamily : queueFamilyProperties)
		{
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
				break;
			selectedQueueFamilyIndex++;
		}

		return selectedQueueFamilyIndex;
	}

	vk::raii::Device createDevice(
		vk::raii::PhysicalDevice &physicalDevice,
		int queueFamilyIndex)
	{
		auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties()[queueFamilyIndex];

		std::vector<float> queuePriorities(
			queueFamilyProperties.queueCount,
			1.f);

		vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
		deviceQueueCreateInfo.setQueueFamilyIndex(queueFamilyIndex);
		deviceQueueCreateInfo.setQueueCount(queueFamilyProperties.queueCount);
		deviceQueueCreateInfo.setQueuePriorities(queuePriorities);

		std::vector<const char *> enabledExtensions{"VK_KHR_swapchain"};

		vk::PhysicalDeviceFeatures physicalDeviceFeatures;
		physicalDeviceFeatures.setSamplerAnisotropy(vk::True);

		vk::DeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.setQueueCreateInfoCount(1);
		deviceCreateInfo.setQueueCreateInfos(deviceQueueCreateInfo);
		deviceCreateInfo.setEnabledExtensionCount(enabledExtensions.size());
		deviceCreateInfo.setPEnabledExtensionNames(enabledExtensions);
		deviceCreateInfo.setPEnabledFeatures(&physicalDeviceFeatures);

		return {
			physicalDevice,
			deviceCreateInfo};
	}

	vk::raii::SurfaceKHR createSurface(
		vk::raii::Instance &instance,
		GLFWwindow *window)
	{
		VkSurfaceKHR surface = VK_NULL_HANDLE;

		glfwCreateWindowSurface(
			static_cast<VkInstance>(*instance),
			window,
			nullptr,
			&surface);

		return {
			instance,
			surface};
	}

	// TODO: change the return type to an optional or throw an exception, other functions should as well
	vk::SurfaceFormatKHR selectSwapChainFormat(
		vk::raii::PhysicalDevice &physicalDevice,
		vk::raii::SurfaceKHR &surface)
	{
		auto formats = physicalDevice.getSurfaceFormatsKHR(*surface);

		for (const auto &format : formats)
		{
			if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
			{
				return format;
			}
		}

		return vk::SurfaceFormatKHR{};
	}

	std::pair<vk::raii::SwapchainKHR, vk::SwapchainCreateInfoKHR> createSwapChain(
		vk::raii::Device &device,
		vk::raii::PhysicalDevice &physicalDevice,
		vk::raii::SurfaceKHR &surface,
		vk::SurfaceFormatKHR format,
		vk::SurfaceCapabilitiesKHR capabilities)
	{
		auto presentModes = physicalDevice.getSurfacePresentModesKHR(*surface);

		vk::PresentModeKHR selectedPresentMode;
		for (const auto &presentMode : presentModes)
		{
			if (presentMode == vk::PresentModeKHR::eMailbox)
			{
				selectedPresentMode = presentMode;
				break;
			}
		}

		vk::SwapchainCreateInfoKHR swapChainCreateInfo;
		swapChainCreateInfo.setPresentMode(selectedPresentMode);
		swapChainCreateInfo.setImageFormat(format.format);
		swapChainCreateInfo.setImageColorSpace(format.colorSpace);
		swapChainCreateInfo.setSurface(*surface);

		swapChainCreateInfo.setImageArrayLayers(1);
		swapChainCreateInfo.setImageSharingMode(vk::SharingMode::eExclusive);
		swapChainCreateInfo.setImageUsage(vk::ImageUsageFlags{vk::ImageUsageFlagBits::eColorAttachment});
		swapChainCreateInfo.setClipped(vk::True);
		swapChainCreateInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
		swapChainCreateInfo.setMinImageCount(capabilities.minImageCount + 1);
		swapChainCreateInfo.setPreTransform(capabilities.currentTransform);

		swapChainCreateInfo.setImageExtent(capabilities.currentExtent);

		return std::pair{
			vk::raii::SwapchainKHR{
				device,
				swapChainCreateInfo},
			swapChainCreateInfo};
	}

	vk::raii::CommandPool createCommandPool(
		vk::raii::Device &device,
		int queueFamilyIndex)
	{
		vk::CommandPoolCreateInfo commandPoolCreateInfo;
		commandPoolCreateInfo.setQueueFamilyIndex(queueFamilyIndex);
		commandPoolCreateInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

		return {
			device,
			commandPoolCreateInfo};
	}

	vk::raii::CommandBuffers createCommandBuffers(
		vk::raii::Device &device,
		vk::raii::CommandPool &commandPool,
		int count)
	{
		vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.setCommandPool(*commandPool);
		commandBufferAllocateInfo.setCommandBufferCount(count);
		commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);

		return {
			device,
			commandBufferAllocateInfo};
	}

	vk::Format selectDepthFormat(vk::raii::PhysicalDevice &physicalDevice)
	{
		std::vector<vk::Format> depthFormats{
			// vk::Format::eD32Sfloat,
			vk::Format::eD32SfloatS8Uint,
			vk::Format::eD24UnormS8Uint};

		std::optional<vk::Format> selectedFormat;
		for (const auto &depthFormat : depthFormats)
		{
			vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(depthFormat);
			if (formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
			{
				selectedFormat = depthFormat;
				break;
			}
		}

		if (!selectedFormat.has_value())
		{
			std::cerr << "Could not find required depth format!" << std::endl;
			throw std::runtime_error("selectDepthFormat");
		}

		return selectedFormat.value();
	}

	std::vector<vk::raii::ImageView> createImageViews(
		vk::raii::Device &device,
		std::vector<vk::Image> &images,
		vk::Format format,
		vk::ImageAspectFlags imageAspectFlags)
	{
		std::vector<vk::raii::ImageView> res;
		res.reserve(images.size());

		for (auto image : images)
		{
			res.push_back(
				createImageView(
					device,
					image,
					format,
					imageAspectFlags));
		}

		return res;
	}

	vk::raii::ImageView createImageView(
		vk::raii::Device &device,
		vk::Image image,
		vk::Format format,
		vk::ImageAspectFlags imageAspectFlags)
	{
		vk::ImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.setFormat(format);
		imageViewCreateInfo.setViewType(vk::ImageViewType::e2D);
		imageViewCreateInfo.setSubresourceRange(
			{imageAspectFlags,
			 0,
			 1,
			 0,
			 1});
		imageViewCreateInfo.setImage(image);

		return {
			device,
			imageViewCreateInfo};
	}

	std::vector<uint32_t> loadShaderCode(const char *path)
	{
		std::ifstream shaderFile(
			path,
			std::ios::in | std::ios::binary);

		std::streamsize size;

		shaderFile.seekg(
			0,
			std::ios::end);
		size = shaderFile.tellg();
		shaderFile.seekg(
			0,
			std::ios::beg);

		std::vector<uint32_t> res;

		if (shaderFile.is_open())
		{
			uint32_t current;
			while (shaderFile.tellg() < size)
			{
				shaderFile.read(
					reinterpret_cast<char *>(&current),
					sizeof(current));
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

	std::vector<vk::raii::ShaderModule> createShaderModules(
		vk::raii::Device &device,
		const char *vertexShaderPath,
		const char *fragmentShaderPath)
	{
		auto vertexShaderCode = loadShaderCode(vertexShaderPath);
		auto fragmentShaderCode = loadShaderCode(fragmentShaderPath);

		vk::ShaderModuleCreateInfo vertexShaderModuleCreateInfo;
		vertexShaderModuleCreateInfo.setCode(vertexShaderCode);
		vertexShaderModuleCreateInfo.setCodeSize(vertexShaderCode.size() * sizeof(uint32_t));

		vk::ShaderModuleCreateInfo fragmentShaderModuleCreateInfo;
		fragmentShaderModuleCreateInfo.setCode(fragmentShaderCode);
		fragmentShaderModuleCreateInfo.setCodeSize(fragmentShaderCode.size() * sizeof(uint32_t));

		std::vector<vk::raii::ShaderModule> res;

		res.emplace_back(
			device,
			vertexShaderModuleCreateInfo);

		res.emplace_back(
			device,
			fragmentShaderModuleCreateInfo);

		return res;
	}

	std::tuple<vk::raii::Pipeline, vk::raii::PipelineLayout, vk::raii::PipelineCache, vk::raii::DescriptorSetLayout>
	createPipeline(
		vk::raii::Device &device,
		vk::raii::RenderPass &renderPass,
		std::vector<vk::raii::ShaderModule> &shaderModules)
	{
		// Shader Stages
		vk::PipelineShaderStageCreateInfo vertexShaderStageCreateInfo;
		vertexShaderStageCreateInfo.setModule(*shaderModules[0]);
		vertexShaderStageCreateInfo.setPName("main");
		vertexShaderStageCreateInfo.setStage(vk::ShaderStageFlagBits::eVertex);

		vk::PipelineShaderStageCreateInfo fragmentShaderStageCreateInfo;
		fragmentShaderStageCreateInfo.setModule(*shaderModules[1]);
		fragmentShaderStageCreateInfo.setPName("main");
		fragmentShaderStageCreateInfo.setStage(vk::ShaderStageFlagBits::eFragment);

		std::vector<vk::PipelineShaderStageCreateInfo> shaderStages{
			vertexShaderStageCreateInfo,
			fragmentShaderStageCreateInfo};

		// Fixed Functions

		// Dynamic State
		std::vector<vk::DynamicState> dynamicStates{
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor};

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

		vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo;
		pipelineMultisampleStateCreateInfo.setSampleShadingEnable(vk::False);
		pipelineMultisampleStateCreateInfo.setRasterizationSamples(vk::SampleCountFlagBits::e1);

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
		colorBlendAttachmentState.setColorWriteMask(
			vk::ColorComponentFlagBits::eR |
			vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA);

		vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo;
		pipelineColorBlendStateCreateInfo.setAttachmentCount(1);
		pipelineColorBlendStateCreateInfo.setAttachments(colorBlendAttachmentState);
		pipelineColorBlendStateCreateInfo.setLogicOpEnable(vk::False);

		// Descriptor Sets Layout
		vk::DescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.setBinding(0);
		uboLayoutBinding.setDescriptorCount(1);
		uboLayoutBinding.setDescriptorType(vk::DescriptorType::eUniformBuffer);
		uboLayoutBinding.setStageFlags(vk::ShaderStageFlagBits::eVertex);

		vk::DescriptorSetLayoutBinding imageSamplerBinding{};
		imageSamplerBinding.setBinding(1);
		imageSamplerBinding.setDescriptorCount(1);
		imageSamplerBinding.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
		imageSamplerBinding.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		std::array<vk::DescriptorSetLayoutBinding, 2> descriptorSetLayoutBindings = {
			uboLayoutBinding,
			imageSamplerBinding};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
		descriptorSetLayoutCreateInfo.setBindingCount(descriptorSetLayoutBindings.size());
		descriptorSetLayoutCreateInfo.setBindings(descriptorSetLayoutBindings);

		vk::raii::DescriptorSetLayout descriptorSetLayout{
			device,
			descriptorSetLayoutCreateInfo};

		// Push Constants
		vk::PushConstantRange pushConstants;
		pushConstants.setOffset(0);
		pushConstants.setSize(sizeof(UniformData));
		pushConstants.setStageFlags(vk::ShaderStageFlagBits::eVertex);

		// Pipeline Layout
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
		pipelineLayoutCreateInfo.setSetLayoutCount(1);
		pipelineLayoutCreateInfo.setSetLayouts(*descriptorSetLayout);
		pipelineLayoutCreateInfo.setPushConstantRanges(pushConstants);
		pipelineLayoutCreateInfo.setPushConstantRangeCount(1);

		vk::raii::PipelineLayout pipelineLayout(
			device,
			pipelineLayoutCreateInfo);

		vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
		graphicsPipelineCreateInfo.setLayout(*pipelineLayout);
		graphicsPipelineCreateInfo.setRenderPass(*renderPass);
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

		vk::PipelineCacheCreateInfo pipelineCacheCreateInfo;
		vk::raii::PipelineCache pipelineCache{
			device,
			pipelineCacheCreateInfo};

		vk::raii::Pipeline pipeline{
			device,
			pipelineCache,
			graphicsPipelineCreateInfo};

		return {
			std::move(pipeline),
			std::move(pipelineLayout),
			std::move(pipelineCache),
			std::move(descriptorSetLayout)};
	}

	std::vector<vk::raii::Framebuffer> createFrameBuffers(
		vk::raii::Device &device,
		vk::raii::RenderPass &renderPass,
		std::vector<vk::raii::ImageView> &imageViews,
		std::vector<vk::raii::ImageView> &depthImageViews,
		vk::SurfaceCapabilitiesKHR surfaceCapabilities)
	{
		std::vector<vk::raii::Framebuffer> frameBuffers;

		vk::FramebufferCreateInfo framebufferCreateInfo;
		framebufferCreateInfo.setRenderPass(*renderPass);
		framebufferCreateInfo.setLayers(1);
		framebufferCreateInfo.setWidth(surfaceCapabilities.currentExtent.width);
		framebufferCreateInfo.setHeight(surfaceCapabilities.currentExtent.height);

		for (int i = 0; i < imageViews.size(); i++)
		{
			std::vector<vk::ImageView> imageViewAttachments{
				*imageViews[i],
				*depthImageViews[i]};

			framebufferCreateInfo.setAttachments(imageViewAttachments);
			framebufferCreateInfo.setAttachmentCount(imageViewAttachments.size());
			frameBuffers.emplace_back(
				device,
				framebufferCreateInfo);
		}

		return frameBuffers;
	}

	vk::raii::RenderPass createRenderPass(
		vk::raii::Device &device,
		vk::SurfaceFormatKHR surfaceFormat,
		vk::Format depthFormat,
		vk::SampleCountFlagBits msaaSamples)
	{
		// Render Pass

		// multisamples color
		vk::AttachmentDescription attachmentDescription;
		attachmentDescription.setLoadOp(vk::AttachmentLoadOp::eClear);
		attachmentDescription.setStoreOp(vk::AttachmentStoreOp::eStore);
		attachmentDescription.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
		attachmentDescription.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
		attachmentDescription.setInitialLayout(vk::ImageLayout::eUndefined);
		attachmentDescription.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
		attachmentDescription.setSamples(msaaSamples);
		attachmentDescription.setFormat(surfaceFormat.format);

		// depth
		vk::AttachmentDescription depthAttachmentDescription{};
		depthAttachmentDescription.setFormat(depthFormat);
		depthAttachmentDescription.setInitialLayout(vk::ImageLayout::eUndefined);
		depthAttachmentDescription.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
		depthAttachmentDescription.setSamples(msaaSamples);
		depthAttachmentDescription.setStoreOp(vk::AttachmentStoreOp::eDontCare);
		depthAttachmentDescription.setLoadOp(vk::AttachmentLoadOp::eClear);
		depthAttachmentDescription.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
		depthAttachmentDescription.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

		// resolved color 1 sample
		vk::AttachmentDescription colorResolve{};
		colorResolve.setFormat(surfaceFormat.format);
		colorResolve.setSamples(msaaSamples);
		colorResolve.setLoadOp(vk::AttachmentLoadOp::eDontCare);
		colorResolve.setStoreOp(vk::AttachmentStoreOp::eStore);
		colorResolve.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
		colorResolve.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
		colorResolve.setInitialLayout(vk::ImageLayout::eUndefined);
		colorResolve.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

		vk::AttachmentReference depthAttachmentReference{};
		depthAttachmentReference.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
		depthAttachmentReference.setAttachment(1);

		vk::AttachmentReference attachmentReference;
		attachmentReference.setAttachment(0);
		attachmentReference.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

		vk::AttachmentReference resolveRef{};
		resolveRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
		resolveRef.setAttachment(2);

		vk::SubpassDescription subpassDescription;
		subpassDescription.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
		subpassDescription.setColorAttachmentCount(1);
		subpassDescription.setColorAttachments(attachmentReference);
		subpassDescription.setPDepthStencilAttachment(&depthAttachmentReference);
		subpassDescription.setResolveAttachments(resolveRef);

		vk::SubpassDependency subpassDependency;
		subpassDependency.setSrcSubpass(vk::SubpassExternal);
		subpassDependency.setDstSubpass(0);
		subpassDependency.setSrcStageMask(
			vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests);
		subpassDependency.setDstStageMask(
			vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests);
		subpassDependency.setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite);
		subpassDependency.setDstAccessMask(
			vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

		std::vector<vk::AttachmentDescription> attachments{
			attachmentDescription,
			depthAttachmentDescription,
			colorResolve};

		vk::RenderPassCreateInfo renderPassCreateInfo;
		renderPassCreateInfo.setAttachmentCount(attachments.size());
		renderPassCreateInfo.setAttachments(attachments);
		renderPassCreateInfo.setSubpassCount(1);
		renderPassCreateInfo.setSubpasses(subpassDescription);
		renderPassCreateInfo.setDependencies(subpassDependency);
		renderPassCreateInfo.setDependencyCount(1);

		return {
			device,
			renderPassCreateInfo};
	}

	vk::raii::DescriptorPool createDescriptorPool(vk::raii::Device &device)
	{
		// TODO fix the sizes for these descriptor pools
		std::vector<vk::DescriptorPoolSize> descriptorPoolSizes{
			{vk::DescriptorType::eSampler,
			 1000},
			{vk::DescriptorType::eCombinedImageSampler,
			 1000},
			{vk::DescriptorType::eSampledImage,
			 1000},
			{vk::DescriptorType::eStorageImage,
			 1000},
			{vk::DescriptorType::eUniformTexelBuffer,
			 1000},
			{vk::DescriptorType::eStorageTexelBuffer,
			 1000},
			{vk::DescriptorType::eUniformBuffer,
			 1000},
			{vk::DescriptorType::eStorageBuffer,
			 1000},
			{vk::DescriptorType::eUniformBufferDynamic,
			 1000},
			{vk::DescriptorType::eStorageBufferDynamic,
			 1000},
			{vk::DescriptorType::eInputAttachment,
			 1000}};

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo{};
		descriptorPoolCreateInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
		descriptorPoolCreateInfo.setPoolSizeCount(descriptorPoolSizes.size());
		descriptorPoolCreateInfo.setPoolSizes(*descriptorPoolSizes.data());
		descriptorPoolCreateInfo.setMaxSets(1000);

		return {
			device,
			descriptorPoolCreateInfo};
	}

	std::vector<vk::raii::DescriptorSet> createDescriptorSets(
		vk::raii::Device &device,
		vk::raii::DescriptorPool &descriptorPool,
		vk::raii::DescriptorSetLayout &descriptorSetLayout,
		int descriptorSetCount)
	{
		std::vector<vk::raii::DescriptorSet> res;

		for (int i = 0; i < descriptorSetCount; i++)
		{
			vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{};
			descriptorSetAllocateInfo.setDescriptorPool(*descriptorPool);
			descriptorSetAllocateInfo.setSetLayouts(*descriptorSetLayout);
			descriptorSetAllocateInfo.setDescriptorSetCount(1);

			res.push_back(std::move(device.allocateDescriptorSets(descriptorSetAllocateInfo)[0]));
		}

		return res;
	}

	std::tuple<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(
		vk::raii::Device &device,
		vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties,
		vk::Flags<vk::MemoryPropertyFlagBits> memoryProperties,
		vk::DeviceSize bufferSize,
		vk::Flags<vk::BufferUsageFlagBits> usage)
	{
		vk::BufferCreateInfo bufferCreateInfo;
		bufferCreateInfo.setUsage(usage);
		bufferCreateInfo.setSharingMode(vk::SharingMode::eExclusive);
		bufferCreateInfo.setSize(bufferSize);

		vk::raii::Buffer buffer{
			device,
			bufferCreateInfo};

		const auto memoryRequirements = buffer.getMemoryRequirements();

		const auto memoryIndex = findMemoryIndex(
			memoryRequirements,
			memoryProperties,
			physicalDeviceMemoryProperties);

		vk::MemoryAllocateInfo memoryAllocateInfo;
		memoryAllocateInfo.setMemoryTypeIndex(memoryIndex);
		memoryAllocateInfo.setAllocationSize(memoryRequirements.size);
		vk::raii::DeviceMemory deviceMemory{
			device,
			memoryAllocateInfo};

		buffer.bindMemory(
			*deviceMemory,
			0);

		return {
			std::move(buffer),
			std::move(deviceMemory)};
	}

	uint32_t findMemoryIndex(
		vk::MemoryRequirements memoryRequirements,
		vk::MemoryPropertyFlags memoryPropertyFlags,
		vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties)
	{
		auto requiredMemoryBits = memoryRequirements.memoryTypeBits;

		for (uint32_t i = 0; physicalDeviceMemoryProperties.memoryTypeCount; i++)
		{
			const uint32_t memoryTypeBits = 1 << i;
			const bool isRequiredMemoryType = memoryTypeBits & requiredMemoryBits;
			if (isRequiredMemoryType &&
				physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & memoryPropertyFlags)
			{
				return i;
			}
		}

		throw std::runtime_error("Failed to find required memory type!");
	}

	std::tuple<vk::raii::Image, vk::raii::DeviceMemory> createImage(
		vk::raii::Device &device,
		vk::raii::PhysicalDevice &physicalDevice,
		vk::ImageTiling tiling,
		vk::ImageUsageFlags imageUsageFlags,
		vk::Format format,
		vk::Extent3D extent,
		vk::ImageType imageType,
		vk::MemoryPropertyFlags memoryPropertyFlags)
	{
		vk::ImageCreateInfo imageCreateInfo{};
		imageCreateInfo.setTiling(tiling);
		imageCreateInfo.setSharingMode(vk::SharingMode::eExclusive);
		imageCreateInfo.setUsage(imageUsageFlags);
		imageCreateInfo.setFormat(format);
		imageCreateInfo.setSamples(vk::SampleCountFlagBits::e1);
		imageCreateInfo.setArrayLayers(1);
		imageCreateInfo.setExtent(extent);
		imageCreateInfo.setImageType(imageType);
		imageCreateInfo.setMipLevels(1);
		imageCreateInfo.setInitialLayout(vk::ImageLayout::eUndefined);

		vk::raii::Image image = device.createImage(imageCreateInfo);
		vk::MemoryRequirements memoryRequirements = image.getMemoryRequirements();
		vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties = physicalDevice.getMemoryProperties();

		const auto memoryIndex = findMemoryIndex(
			memoryRequirements,
			memoryPropertyFlags,
			physicalDeviceMemoryProperties);

		vk::MemoryAllocateInfo memoryAllocateInfo;
		memoryAllocateInfo.setMemoryTypeIndex(memoryIndex);
		memoryAllocateInfo.setAllocationSize(memoryRequirements.size);

		vk::raii::DeviceMemory imageMemory = device.allocateMemory(memoryAllocateInfo);

		image.bindMemory(
			*imageMemory,
			0);

		return {
			std::move(image),
			std::move(imageMemory)};
	}
	Image createImage2(
		const VmaAllocator &allocator,
		vk::Format format,
		vk::Extent3D extent,
		int mipLevels,
		vk::SampleCountFlagBits sampleCount,
		vk::ImageUsageFlags imageUsage)
	{
		vk::ImageCreateInfo imageCreateInfo{};
		imageCreateInfo.setArrayLayers(1);
		imageCreateInfo.setFormat(format);
		imageCreateInfo.setExtent(extent);
		imageCreateInfo.setImageType(vk::ImageType::e2D);
		imageCreateInfo.setInitialLayout(vk::ImageLayout::eUndefined);
		imageCreateInfo.setMipLevels(static_cast<uint32_t>(mipLevels));
		imageCreateInfo.setSamples(sampleCount);
		imageCreateInfo.setSharingMode(vk::SharingMode::eExclusive);
		imageCreateInfo.setTiling(vk::ImageTiling::eOptimal);
		imageCreateInfo.setUsage(imageUsage);

		VmaAllocationCreateInfo allocationCreateInfo;
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocationCreateInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

		Image res{};
		VkImage image;
		vmaCreateImage(allocator, &static_cast<VkImageCreateInfo>(imageCreateInfo), &allocationCreateInfo, &image, &res.allocation, nullptr);
		res.handle = vk::Image{image};
		res.createInfo = imageCreateInfo;

		return res;
	}

	vk::ImageView createImageView2(
		const vk::Device &device,
		const vk::Image &image,
		vk::Format format,
		vk::ImageAspectFlags imageAspectFlags)
	{
		vk::ImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.setFormat(format);
		imageViewCreateInfo.setViewType(vk::ImageViewType::e2D);
		imageViewCreateInfo.setSubresourceRange(
			{imageAspectFlags,
			 0,
			 1,
			 0,
			 1});
		imageViewCreateInfo.setImage(image);

		return device.createImageView(imageViewCreateInfo);
	}
} // pbr

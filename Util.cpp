#include "Util.h"

namespace util {
	vk::raii::Instance createInstance(
			vk::raii::Context &context,
			const char *applicationName,
			const char *engineName
	) {
		std::vector<const char *> enabledLayers{};
		std::vector<const char *> enabledExtensions{};

		// Add GLFW necessary extensions
		uint32_t glfwExtensionCount = 0;
		const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		enabledExtensions.reserve(glfwExtensionCount);
		for (int i = 0; i < static_cast<int>(glfwExtensionCount); i++) {
			enabledExtensions.push_back(glfwExtensions[i]);
		}

		vk::ApplicationInfo applicationInfo(
				applicationName,
				1,
				engineName,
				1,
				vk::ApiVersion13
		);


		vk::InstanceCreateInfo instanceCreateInfo(
				{},
				&applicationInfo,
				enabledLayers.size(),
				enabledLayers.data(),
				enabledExtensions.size(),
				enabledExtensions.data(),
				nullptr
		);

		return context.createInstance(instanceCreateInfo);
	}

	vk::raii::PhysicalDevice selectPhysicalDevice(
			vk::raii::Instance &instance
	) {
		auto physicalDevices = instance.enumeratePhysicalDevices();

		vk::raii::PhysicalDevice selectedDevice(nullptr);
		for (const auto &pDevice: physicalDevices) {
			if (pDevice.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
				selectedDevice = pDevice;
				break;
			}
		}

		std::cout << "Selected Device: " << selectedDevice.getProperties().deviceName << std::endl;

		return selectedDevice;
	}

	int selectQueueFamily(
			vk::raii::PhysicalDevice &physicalDevice
	) {
		auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

		int selectedQueueFamilyIndex = 0;
		for (const auto &queueFamily: queueFamilyProperties) {
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) break;
			selectedQueueFamilyIndex++;
		}

		return selectedQueueFamilyIndex;
	}

	vk::raii::Device createDevice(
			vk::raii::PhysicalDevice &physicalDevice,
			int queueFamilyIndex
	) {
		auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties()[queueFamilyIndex];

		std::vector<float> queuePriorities(
				queueFamilyProperties.queueCount,
				1.f
		);

		vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
		deviceQueueCreateInfo.setQueueFamilyIndex(queueFamilyIndex);
		deviceQueueCreateInfo.setQueueCount(queueFamilyProperties.queueCount);
		deviceQueueCreateInfo.setQueuePriorities(queuePriorities);

		std::vector<const char *> enabledExtensions{"VK_KHR_swapchain"};

		vk::DeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.setQueueCreateInfoCount(1);
		deviceCreateInfo.setQueueCreateInfos(deviceQueueCreateInfo);
		deviceCreateInfo.setEnabledExtensionCount(enabledExtensions.size());
		deviceCreateInfo.setPEnabledExtensionNames(enabledExtensions);

		return {physicalDevice, deviceCreateInfo};
	}

	vk::raii::SurfaceKHR createSurface(
			vk::raii::Instance &instance,
			GLFWwindow *window
	) {
		VkSurfaceKHR surface = VK_NULL_HANDLE;

		glfwCreateWindowSurface(
				static_cast<VkInstance>(*instance),
				window,
				nullptr,
				&surface
		);

		return {instance, surface};
	}

	// TODO: change the return type to an optional or throw an exception, other functions should as well
	vk::SurfaceFormatKHR selectSwapChainFormat(
			vk::raii::PhysicalDevice &physicalDevice,
			vk::raii::SurfaceKHR &surface
	) {
		auto formats = physicalDevice.getSurfaceFormatsKHR(*surface);

		for (const auto &format: formats) {
			if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
				return format;
			}
		}

		return vk::SurfaceFormatKHR{};
	}

	vk::raii::SwapchainKHR createSwapChain(
			vk::raii::Device &device,
			vk::raii::PhysicalDevice &physicalDevice,
			vk::raii::SurfaceKHR &surface,
			vk::SurfaceFormatKHR format,
			vk::SurfaceCapabilitiesKHR capabilities
	) {
		auto presentModes = physicalDevice.getSurfacePresentModesKHR(*surface);

		vk::PresentModeKHR selectedPresentMode;
		for (const auto &presentMode: presentModes) {
			if (presentMode == vk::PresentModeKHR::eMailbox) {
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

		return {device, swapChainCreateInfo};
	}

	vk::raii::CommandPool createCommandPool(
			vk::raii::Device &device,
			int queueFamilyIndex
	) {
		vk::CommandPoolCreateInfo commandPoolCreateInfo;
		commandPoolCreateInfo.setQueueFamilyIndex(queueFamilyIndex);

		return {device, commandPoolCreateInfo};
	}

	vk::raii::CommandBuffers createCommandBuffers(
			vk::raii::Device &device,
			vk::raii::CommandPool &commandPool,
			int count
	) {
		vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.setCommandPool(*commandPool);
		commandBufferAllocateInfo.setCommandBufferCount(count);
		commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);

		return {device, commandBufferAllocateInfo};
	}

	std::vector<vk::raii::ImageView> createImageViews(
			vk::raii::Device &device,
			std::vector<vk::Image> &images,
			vk::SurfaceFormatKHR &swapChainFormat
	) {
		std::vector<vk::raii::ImageView> res;
		res.reserve(images.size());

		vk::ImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.setComponents({});
		imageViewCreateInfo.setFormat(swapChainFormat.format);
		imageViewCreateInfo.setViewType(vk::ImageViewType::e2D);
		imageViewCreateInfo.setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

		for (auto image: images) {
			imageViewCreateInfo.setImage(image);
			res.emplace_back(
					device,
					imageViewCreateInfo
			);
		}

		return res;
	}

	std::vector<uint32_t> loadShaderCode(const char *path) {
		std::ifstream shaderFile(
				path,
				std::ios::in | std::ios::binary
		);

		std::streamsize size;

		shaderFile.seekg(
				0,
				std::ios::end
		);
		size = shaderFile.tellg();
		shaderFile.seekg(
				0,
				std::ios::beg
		);

		std::vector<uint32_t> res;

		if (shaderFile.is_open()) {
			uint32_t current;
			while (shaderFile.tellg() < size) {
				shaderFile.read(
						reinterpret_cast<char *>(&current),
						sizeof(current)
				);
				res.push_back(current);
			}
		} else {
			std::cerr << "Could not open shader file!" << std::endl;
			throw std::runtime_error(path);
		}

		shaderFile.close();

		return res;
	}

	std::vector<vk::raii::ShaderModule> createShaderModules(
			vk::raii::Device &device,
			const char *vertexShaderPath,
			const char *fragmentShaderPath
	) {
		auto vertexShaderCode = loadShaderCode(vertexShaderPath);
		auto fragmentShaderCode = loadShaderCode(fragmentShaderPath);

		vk::ShaderModuleCreateInfo vertexShaderModuleCreateInfo;
		vertexShaderModuleCreateInfo.setCode(vertexShaderCode);
		vertexShaderModuleCreateInfo.setCodeSize(vertexShaderCode.size());

		vk::ShaderModuleCreateInfo fragmentShaderModuleCreateInfo;
		fragmentShaderModuleCreateInfo.setCode(fragmentShaderCode);
		fragmentShaderModuleCreateInfo.setCodeSize(fragmentShaderCode.size());

		std::vector<vk::raii::ShaderModule> res;

		res.emplace_back(
				device,
				vertexShaderModuleCreateInfo
		);

		res.emplace_back(
				device,
				fragmentShaderModuleCreateInfo
		);

		return res;
	}

	vk::raii::Pipeline createPipeline(
			vk::raii::Device &device,
			std::vector<vk::raii::ShaderModule> shaderModules,
			vk::SurfaceCapabilitiesKHR surfaceCapabilities
	) {
		// Shader Stages
		vk::PipelineShaderStageCreateInfo vertexShaderStageCreateInfo;
		vertexShaderStageCreateInfo.setModule(*shaderModules[0]);
		vertexShaderStageCreateInfo.setPName("main");
		vertexShaderStageCreateInfo.setStage(vk::ShaderStageFlagBits::eVertex);

		vk::PipelineShaderStageCreateInfo fragmentShaderStageCreateInfo;
		fragmentShaderStageCreateInfo.setModule(*shaderModules[1]);
		fragmentShaderStageCreateInfo.setPName("main");
		fragmentShaderStageCreateInfo.setStage(vk::ShaderStageFlagBits::eFragment);

		// Fixed Functions

		// Dynamic State
		std::vector<vk::DynamicState> dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};

		vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo;
		pipelineDynamicStateCreateInfo.setDynamicStateCount(dynamicStates.size());
		pipelineDynamicStateCreateInfo.setDynamicStates(dynamicStates);

		// Vertex Input
		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;

		// Input Assembly
		vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo;
		pipelineInputAssemblyStateCreateInfo.setPrimitiveRestartEnable(vk::False);
		pipelineInputAssemblyStateCreateInfo.setTopology(vk::PrimitiveTopology::eTriangleList);

		// Viewport
		vk::Viewport viewport;
		viewport.setX(0.f);
		viewport.setY(0.f);
		viewport.setMaxDepth(1.f);
		viewport.setMinDepth(0.f);
		viewport.setWidth(static_cast<float>(surfaceCapabilities.currentExtent.width));
		viewport.setHeight(static_cast<float>(surfaceCapabilities.currentExtent.height));

		vk::Rect2D scissor;
		scissor.setOffset({0, 0});
		scissor.setExtent(surfaceCapabilities.currentExtent);

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
		pipelineRasterizationStateCreateInfo.setFrontFace(vk::FrontFace::eClockwise);
		pipelineRasterizationStateCreateInfo.setDepthBiasEnable(vk::False);

		// Multisampling

		vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo;
		pipelineMultisampleStateCreateInfo.setSampleShadingEnable(vk::False);
		pipelineMultisampleStateCreateInfo.setRasterizationSamples(vk::SampleCountFlagBits::e1);

		// Depth testing

		// Color blending
		vk::PipelineColorBlendAttachmentState colorBlendAttachmentState;
		colorBlendAttachmentState.setBlendEnable(vk::False);
		colorBlendAttachmentState.setColorWriteMask(
				vk::ColorComponentFlagBits::eR |
				vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB |
				vk::ColorComponentFlagBits::eA
		);

		vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo;
		pipelineColorBlendStateCreateInfo.setAttachmentCount(1);
		pipelineColorBlendStateCreateInfo.setAttachments(colorBlendAttachmentState);
		pipelineColorBlendStateCreateInfo.setLogicOpEnable(vk::False);

		// Pipeline Layout
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;

		vk::raii::PipelineLayout pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);
	}
} // pbr

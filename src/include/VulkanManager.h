#ifndef VULKAN_MANAGER_H_
#define VULKAN_MANAGER_H_

#include <Util.h>

#include <glfw/glfw3.h>

struct VulkanState
{
	std::unique_ptr<vk::raii::Context> context;
	std::unique_ptr<vk::raii::PhysicalDevice> physicalDevice;
	std::unique_ptr<vk::raii::Instance> instance;
	std::unique_ptr<vk::raii::Device> device;
	std::unique_ptr<vk::raii::SurfaceKHR> surface;
	std::unique_ptr<vk::raii::SwapchainKHR> swapChain;
	std::unique_ptr<std::vector<vk::Image>> swapChainImages;
	std::unique_ptr<std::vector<vk::raii::ImageView>> swapChainImageViews;
	std::unique_ptr<vk::raii::CommandPool> commandPool;
	std::unique_ptr<std::vector<vk::raii::CommandBuffer>> commandBuffers;
	std::unique_ptr<std::vector<vk::raii::ShaderModule>> shaderModules;

	std::unique_ptr<vk::raii::DebugUtilsMessengerEXT> debugMessenger;

	int queueFamilyGraphicsIndex;
	int maxFramesInFlight;
	vk::SurfaceFormatKHR swapChainFormat;
	vk::SurfaceCapabilitiesKHR surfaceCapabilities;
	vk::SwapchainCreateInfoKHR swapChainCreateInfo;
};

class VulkanManager
{
public:
	VulkanManager(GLFWwindow *window, int maxFramesInFlight);
	VulkanManager(const VulkanManager &rhs) = delete;
	VulkanManager(const VulkanManager &&rhs) = delete;
	VulkanState &getVulkanState();

private:
	VulkanState vkState{};
};

#endif

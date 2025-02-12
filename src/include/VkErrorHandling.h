#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_to_string.hpp>
#include <vulkan/vk_enum_string_helper.h>

#include "ansi_color_defs.h"

inline void checkResult(vk::Result res)
{
	if (res != vk::Result::eSuccess)
	{
		std::cerr << "Encountered a Vulkan error:\n";
		std::cerr << vk::to_string(res) << std::endl;
		throw std::runtime_error("Vulkan runtime exception");
	}
}

inline void checkResult(VkResult res)
{
	if (res != VkResult::VK_SUCCESS)
	{
		std::cerr << "Encountered a Vulkan error:\n";
		std::cerr << string_VkResult(res) << std::endl;
		throw std::runtime_error("Vulkan runtime exception");
	}
}

class VkResCheck
{
  public:
	VkResCheck() = default;

	VkResCheck(const vk::Result &rhs)
	{
		lastResult = rhs;
		checkResult(rhs);
	}

	VkResCheck(const VkResult &rhs)
	{
		lastResult = vk::Result(rhs);
		checkResult(lastResult);
	}

	static void checkResult(vk::Result result)
	{
		if (result != vk::Result::eSuccess)
		{
			std::cerr << "Encountered a Vulkan error:\n";
			std::cerr << vk::to_string(result) << std::endl;
			throw std::runtime_error("Vulkan runtime exception");
		}
	}

	static VkBool32 VKAPI_PTR PFN_vkDebugUtilsMessengerCallbackEXT(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
	{
		if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			ANSI_COLOR_BLUE
			std::cout << "Info: " << pCallbackData->pMessage << std::endl;
			ANSI_COLOR_RESET
		}
		if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			ANSI_COLOR_YELLOW
			std::cout << "Warning: " << pCallbackData->pMessage << std::endl;
			ANSI_COLOR_RESET
		}
		if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			ANSI_COLOR_RED
			std::cout << "Error: " << pCallbackData->pMessage << std::endl;
			ANSI_COLOR_RESET
		}
		if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		{
			ANSI_COLOR_WHITE
			std::cout << "Verbose: " << pCallbackData->pMessage << std::endl;
			ANSI_COLOR_RESET
		}
		std::cout << std::endl;
		return VK_FALSE;
	}

  private:
	vk::Result lastResult{};
};

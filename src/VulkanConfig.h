#ifndef COMMON_VULKAN_CONFIG_H
#define COMMON_VULKAN_CONFIG_H

#include "VulkanEnvironment.h"

#include <vector>

#include <vulkan/vulkan.h>

namespace VulkanConfig
{
	const char appName[] = u8"Voxel Engine";

#if VULKAN_VALIDATION
  const VkDebugUtilsMessageSeverityFlagsEXT debugSeverity =
    0
    //| VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
    //| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
  ;

  const VkDebugUtilsMessageTypeFlagsEXT debugType =
		0
    | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
    | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
    | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
  ;

	constexpr bool useAssistantLayer = false;
#endif

	constexpr bool fpsCounter = true;
	
// window and swapchain
	constexpr uint32_t initialWindowWidth = 800;
	constexpr uint32_t initialWindowHeight = 800;
	
//constexpr VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // better not be used often because of coil whine
	constexpr VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
//constexpr VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	
// pipeline settings
	constexpr VkClearValue clearColor = {  { {0.1f, 0.1f, 0.1f, 1.0f} }  };
	
// Makes present queue from different Queue Family than Graphics, for testing purposes
	constexpr bool forceSeparatePresentQueue = false;
}

#endif //COMMON_VULKAN_CONFIG_H

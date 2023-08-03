// Vulkan extensions commands loader

#ifndef EXTENSION_LOADER_H
#define EXTENSION_LOADER_H

#include <vector>

#include <unordered_map>

#include <cstring>

#include <vulkan/vulkan.h>

#include "CompilerMessages.h"
#include "EnumerateScheme.h"

void loadInstanceExtensionsCommands( VkInstance instance, const std::vector<const char*>& instanceExtensions );
void unloadInstanceExtensionsCommands( VkInstance instance );

void loadDeviceExtensionsCommands( VkDevice device, const std::vector<const char*>& instanceExtensions );
void unloadDeviceExtensionsCommands( VkDevice device );

void loadPDProps2Commands( VkInstance instance );
void unloadPDProps2Commands( VkInstance instance );

void loadDebugReportCommands( VkInstance instance );
void unloadDebugReportCommands( VkInstance instance );

void loadDebugUtilsCommands( VkInstance instance );
void unloadDebugUtilsCommands( VkInstance instance );

void loadExternalMemoryCapsCommands( VkInstance instance );
void unloadExternalMemoryCapsCommands( VkInstance instance );


void loadExternalMemoryCommands( VkDevice device );
void unloadExternalMemoryCommands( VkDevice device );

#ifdef VK_USE_PLATFORM_WIN32_KHR
void loadExternalMemoryWin32Commands( VkDevice device );
void unloadExternalMemoryWin32Commands( VkDevice device );
#endif

void loadDedicatedAllocationCommands( VkDevice device );
void unloadDedicatedAllocationCommands( VkDevice device );

////////////////////////////////////////////////////////

static std::unordered_map< VkInstance, std::vector<const char*> > instanceExtensionsMap;
static std::unordered_map< VkPhysicalDevice, VkInstance > physicalDeviceInstanceMap;
static std::unordered_map< VkDevice, std::vector<const char*> > deviceExtensionsMap;

TODO( "Leaks destroyed instances" );
void populatePhysicalDeviceInstaceMap( const VkInstance instance );

void loadInstanceExtensionsCommands( const VkInstance instance, const std::vector<const char*>& instanceExtensions );
void unloadInstanceExtensionsCommands( const VkInstance instance );

void loadDeviceExtensionsCommands( const VkDevice device, const std::vector<const char*>& deviceExtensions );
void unloadDeviceExtensionsCommands( const VkDevice device );


// VK_KHR_get_physical_device_properties2
///////////////////////////////////////////

static std::unordered_map< VkInstance, PFN_vkGetPhysicalDeviceFeatures2KHR > GetPhysicalDeviceFeatures2KHRDispatchTable;
static std::unordered_map< VkInstance, PFN_vkGetPhysicalDeviceProperties2KHR > GetPhysicalDeviceProperties2KHRDispatchTable;
static std::unordered_map< VkInstance, PFN_vkGetPhysicalDeviceFormatProperties2KHR > GetPhysicalDeviceFormatProperties2KHRDispatchTable;
static std::unordered_map< VkInstance, PFN_vkGetPhysicalDeviceImageFormatProperties2KHR > GetPhysicalDeviceImageFormatProperties2KHRDispatchTable;
static std::unordered_map< VkInstance, PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR > GetPhysicalDeviceQueueFamilyProperties2KHRDispatchTable;
static std::unordered_map< VkInstance, PFN_vkGetPhysicalDeviceMemoryProperties2KHR > GetPhysicalDeviceMemoryProperties2KHRDispatchTable;
static std::unordered_map< VkInstance, PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR > GetPhysicalDeviceSparseImageFormatProperties2KHRDispatchTable;

void loadPDProps2Commands( VkInstance instance );
void unloadPDProps2Commands( VkInstance instance );

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures2KHR( VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2* pFeatures );

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties2KHR( VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2* pProperties );

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties2KHR( VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2* pFormatProperties );

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties2KHR( VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo, VkImageFormatProperties2* pImageFormatProperties );

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties2KHR( VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties2* pQueueFamilyProperties );

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties2KHR( VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2* pMemoryProperties );

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties2KHR( VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2* pFormatInfo, uint32_t* pPropertyCount, VkSparseImageFormatProperties2* pProperties );

// VK_EXT_debug_report
//////////////////////////////////

static std::unordered_map< VkInstance, PFN_vkCreateDebugReportCallbackEXT > CreateDebugReportCallbackEXTDispatchTable;
static std::unordered_map< VkInstance, PFN_vkDestroyDebugReportCallbackEXT > DestroyDebugReportCallbackEXTDispatchTable;
static std::unordered_map< VkInstance, PFN_vkDebugReportMessageEXT > DebugReportMessageEXTDispatchTable;

void loadDebugReportCommands( VkInstance instance );

void unloadDebugReportCommands( VkInstance instance );

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(
	VkInstance instance,
	const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugReportCallbackEXT* pCallback
);

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(
	VkInstance instance,
	VkDebugReportCallbackEXT callback,
	const VkAllocationCallbacks* pAllocator
);

VKAPI_ATTR void VKAPI_CALL vkDebugReportMessageEXT(
	VkInstance instance,
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t object,
	size_t location,
	int32_t messageCode,
	const char* pLayerPrefix,
	const char* pMessage
);

// VK_EXT_debug_utils
//////////////////////////////////

static std::unordered_map< VkInstance, PFN_vkCreateDebugUtilsMessengerEXT > CreateDebugUtilsMessengerEXTDispatchTable;
static std::unordered_map< VkInstance, PFN_vkDestroyDebugUtilsMessengerEXT > DestroyDebugUtilsMessengerEXTDispatchTable;
static std::unordered_map< VkInstance, PFN_vkSubmitDebugUtilsMessageEXT > SubmitDebugUtilsMessageEXTDispatchTable;

void loadDebugUtilsCommands( VkInstance instance );
void unloadDebugUtilsCommands( VkInstance instance );

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pMessenger
);

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT messenger,
	const VkAllocationCallbacks* pAllocator
);

VKAPI_ATTR void VKAPI_CALL vkSubmitDebugUtilsMessageEXT(
	VkInstance instance,
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
);

// VK_KHR_external_memory_capabilities
///////////////////////////////////////////

static std::unordered_map< VkInstance, PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR > GetPhysicalDeviceExternalBufferPropertiesKHRDispatchTable;

void loadExternalMemoryCapsCommands( VkInstance instance );
void unloadExternalMemoryCapsCommands( VkInstance instance );

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalBufferPropertiesKHR(
	VkPhysicalDevice physicalDevice,
	const VkPhysicalDeviceExternalBufferInfo* pExternalBufferInfo,
	VkExternalBufferProperties* pExternalBufferProperties
);

// VK_KHR_external_memory
///////////////////////////////////////////

void loadExternalMemoryCommands( VkDevice );
void unloadExternalMemoryCommands( VkDevice );

#ifdef VK_USE_PLATFORM_WIN32_KHR
// VK_KHR_external_memory_win32
///////////////////////////////////////////

static std::unordered_map< VkDevice, PFN_vkGetMemoryWin32HandleKHR > GetMemoryWin32HandleKHRDispatchTable;
static std::unordered_map< VkDevice, PFN_vkGetMemoryWin32HandlePropertiesKHR > GetMemoryWin32HandlePropertiesKHRDispatchTable;

void loadExternalMemoryWin32Commands( VkDevice device );
void unloadExternalMemoryWin32Commands( VkDevice device );

VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryWin32HandleKHR(
	VkDevice device,
	const VkMemoryGetWin32HandleInfoKHR* pGetWin32HandleInfo,
	HANDLE* pHandle
);

VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryWin32HandlePropertiesKHR(
	VkDevice device,
	VkExternalMemoryHandleTypeFlagBits handleType,
	HANDLE handle,
	VkMemoryWin32HandlePropertiesKHR* pMemoryWin32HandleProperties
);
#endif

// VK_KHR_dedicated_allocation
///////////////////////////////////////////

void loadDedicatedAllocationCommands( VkDevice );
void unloadDedicatedAllocationCommands( VkDevice );

#endif //EXTENSION_LOADER_H

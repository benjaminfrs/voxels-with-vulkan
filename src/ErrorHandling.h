// Reusable error handling primitives for Vulkan

#ifndef COMMON_ERROR_HANDLING_H
#define COMMON_ERROR_HANDLING_H

#include <iostream>
#include <string>
#include <sstream>

#include <vulkan/vulkan.h>

#include "VulkanIntrospection.h"

//Exception Handlers
struct VulkanResultException{
	const char* file;
	unsigned line;
	const char* func;
	const char* source;
	VkResult result;
	VulkanResultException( const char* file, unsigned line, const char* func, const char* source, VkResult result )
	: file( file ), line( line ), func( func ), source( source ), result( result ){}
};

void RESULT_HANDLER( VkResult errorCode, const char* source );
void RESULT_HANDLER_EX( uint32_t cond, VkResult errorCode, const char* source );
void RUNTIME_ASSERT( uint32_t cond, VkResult errorCode, const char* source );

// just use cout for logging now
static std::ostream& logger = std::cout;

enum class Highlight;
void genericDebugCallback( std::string flags, Highlight highlight, std::string msgCode, std::string object, const char* message );

VKAPI_ATTR VkBool32 VKAPI_CALL genericDebugReportCallback(
	VkDebugReportFlagsEXT msgFlags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t srcObject,
	size_t /*location*/,
	int32_t msgCode,
	const char* pLayerPrefix,
	const char* pMsg,
	void* /*pUserData*/
);

VKAPI_ATTR VkBool32 VKAPI_CALL genericDebugUtilsCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* /*pUserData*/
);

enum class DebugObjectType { debugReport, debugUtils };

struct DebugObjectVariant{
	DebugObjectType tag;
	union{
		VkDebugReportCallbackEXT debugReportCallback;
		VkDebugUtilsMessengerEXT debugUtilsMessenger;
	};
};

DebugObjectVariant initDebug( const VkInstance instance, const DebugObjectType debugExtension, const VkDebugUtilsMessageSeverityFlagsEXT debugSeverity, const VkDebugUtilsMessageTypeFlagsEXT debugType );
void killDebug( VkInstance instance, DebugObjectVariant debug );

VkDebugReportFlagsEXT translateFlags( const VkDebugUtilsMessageSeverityFlagsEXT debugSeverity, const VkDebugUtilsMessageTypeFlagsEXT debugType );

// Implementation
//////////////////////////////////

void genericDebugCallback( std::string flags, Highlight highlight, std::string msgCode, std::string object, const char* message );

VKAPI_ATTR VkBool32 VKAPI_CALL genericDebugReportCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t object,
	size_t /*location*/,
	int32_t messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* /*pUserData*/
);

VKAPI_ATTR VkBool32 VKAPI_CALL genericDebugUtilsCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* /*pUserData*/
);

VkDebugReportFlagsEXT translateFlags( const VkDebugUtilsMessageSeverityFlagsEXT debugSeverity, const VkDebugUtilsMessageTypeFlagsEXT debugType );

DebugObjectVariant initDebug( const VkInstance instance, const DebugObjectType debugExtension, const VkDebugUtilsMessageSeverityFlagsEXT debugSeverity, const VkDebugUtilsMessageTypeFlagsEXT debugType );

void killDebug( const VkInstance instance, const DebugObjectVariant debug );

#endif //COMMON_ERROR_HANDLING_H

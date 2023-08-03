// Introspection for Vulkan enums -- mostly to_string

#ifndef COMMON_VULKAN_INTROSPECTION_H
#define COMMON_VULKAN_INTROSPECTION_H

#include <string>
#include <sstream>

#include <vulkan/vulkan.h>


template <typename PHANDLE_T>
inline uint64_t handleToUint64(const PHANDLE_T *h) { return reinterpret_cast<uint64_t>(h); }
inline uint64_t handleToUint64(const uint64_t h) { return h; }

const char* to_string( const VkResult r );
std::string to_string( const VkDebugReportObjectTypeEXT o );
std::string to_string( const VkObjectType o );
std::string to_string( const VkDebugUtilsMessageSeverityFlagBitsEXT debugSeverity );

std::string to_string_hex( const uint64_t n );

std::string dbrflags_to_string( VkDebugReportFlagsEXT msgFlags );
std::string dbuseverity_to_string( const VkDebugUtilsMessageSeverityFlagsEXT debugSeverity );
std::string dbutype_to_string( const VkDebugUtilsMessageTypeFlagsEXT debugType );

#endif //COMMON_VULKAN_INTROSPECTION_H

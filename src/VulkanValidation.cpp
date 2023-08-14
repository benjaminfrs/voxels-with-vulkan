#include "VulkanEnvironment.h"

#include <fstream>
#include <vector>

#include <VulkanValidation.h>


#include <vulkan/vulkan.h>
#include <EnumerateScheme.h>
#include <VulkanConfig.h>
#include <VulkanImpl.h>
#include <Glfw.h>

#include <vector>

using std::vector;
using std::string;
using std::to_string;

VulkanManager :: VulkanManager()
{
  const auto supportedLayers = enumerate<VkInstance, VkLayerProperties>();

#if VULKAN_VALIDATION
  if(  isLayerSupported( "VK_LAYER_KHRONOS_validation", supportedLayers )  ) m_requestedLayers.push_back( "VK_LAYER_KHRONOS_validation" );
  else throw "VULKAN_VALIDATION is enabled but VK_LAYER_KHRONOS_validation layers are not supported!";

  if( VulkanConfig::useAssistantLayer ){
    if(  isLayerSupported( "VK_LAYER_LUNARG_assistant_layer", supportedLayers )  ) m_requestedLayers.push_back( "VK_LAYER_LUNARG_assistant_layer" );
    else throw "VULKAN_VALIDATION is enabled but VK_LAYER_LUNARG_assistant_layer layer is not supported!";
  }
#endif

  if( VulkanConfig::fpsCounter ) m_requestedLayers.push_back( "VK_LAYER_LUNARG_monitor" );
  m_requestedLayers = checkInstanceLayerSupport( m_requestedLayers, supportedLayers );


  const auto supportedInstanceExtensions = getSupportedInstanceExtensions( m_requestedLayers );
  const auto platformSurfaceExtension = getPlatformSurfaceExtensionName();
  vector<const char*> requestedInstanceExtensions = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    platformSurfaceExtension.c_str(),
#ifdef __APPLE__
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
#endif
  };

#if VULKAN_VALIDATION
  DebugObjectType debugExtensionTag;
  if(  isExtensionSupported( VK_EXT_DEBUG_UTILS_EXTENSION_NAME, supportedInstanceExtensions )  ){
    debugExtensionTag = DebugObjectType::debugUtils;
    requestedInstanceExtensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
  }
  else if(  isExtensionSupported( VK_EXT_DEBUG_REPORT_EXTENSION_NAME, supportedInstanceExtensions )  ){
    debugExtensionTag = DebugObjectType::debugReport;
    requestedInstanceExtensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
  }
  else throw "VULKAN_VALIDATION is enabled but neither VK_EXT_debug_utils nor VK_EXT_debug_report extension is supported!";
#endif

  checkExtensionSupport( requestedInstanceExtensions, supportedInstanceExtensions );
  m_vkInstance = initInstance( m_requestedLayers, requestedInstanceExtensions );

#if VULKAN_VALIDATION
  m_debugHandle = initDebug( m_vkInstance, debugExtensionTag, VulkanConfig::debugSeverity, VulkanConfig::debugType );

  const int32_t uncoded = 0;
  const char* introMsg = "Validation Layers are enabled!";
  if( debugExtensionTag == DebugObjectType::debugUtils ){
    VkDebugUtilsObjectNameInfoEXT object = {
      VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
      nullptr, // pNext
      VK_OBJECT_TYPE_INSTANCE,
      handleToUint64(m_vkInstance),
      "instance"
    };
    const VkDebugUtilsMessengerCallbackDataEXT dumcd = {
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT,
      nullptr, // pNext
      0, // flags
      "VULKAN_VALIDATION", // VUID
      0, // VUID hash
      introMsg,
      0, nullptr, 0, nullptr,
      1, &object
    };
    vkSubmitDebugUtilsMessageEXT( m_vkInstance, VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, &dumcd );
  }
  else if( debugExtensionTag == DebugObjectType::debugReport ){
    vkDebugReportMessageEXT( m_vkInstance, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, (uint64_t)m_vkInstance, __LINE__, uncoded, "Application", introMsg );
  }
#endif
}

VkInstance VulkanManager :: getVkInstance() { return m_vkInstance; }
DebugObjectVariant VulkanManager :: getDebugHandle() { return m_debugHandle; }
vector<const char*> VulkanManager :: getRequestedLayers() { return m_requestedLayers; }

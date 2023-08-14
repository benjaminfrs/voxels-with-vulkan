#ifndef COMMON_VULKAN_VALIDATION_H
#define COMMON_VULKAN_VALIDATION_H

#include <vector>

#include <vulkan/vulkan.h>
#include <EnumerateScheme.h>

using std::vector;


class VulkanManager
{
public:
  //No copy constructor
  VulkanManager(const VulkanManager& manager) = delete; 

  VulkanManager();

  VkInstance getVkInstance();
  DebugObjectVariant getDebugHandle();
  vector<const char*> getRequestedLayers();
private:
  vector<const char*> m_requestedLayers;
  VkInstance m_vkInstance; 
  DebugObjectVariant m_debugHandle;
};

#endif //COMMON_VULKAN_VALIDATION_H

#include "VulkanEnvironment.h"

#include <fstream>
#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanImpl.h"

#include "VulkanConfig.h"
#include "EnumerateScheme.h"
#include "ErrorHandling.h"
#include "ExtensionLoader.h"
#include "Vertex.h"
#include "Wsi.h"

using std::exception;
using std::string;
using std::to_string;
using std::vector;
// Implementation
//////////////////////////////////////////////////////////////////////////////////

bool isLayerSupported( const char* layer, const vector<VkLayerProperties>& supportedLayers ){
	const auto isSupportedPred = [layer]( const VkLayerProperties& prop ) -> bool{
		return std::strcmp( layer, prop.layerName ) == 0;
	};

	return std::any_of( supportedLayers.begin(), supportedLayers.end(), isSupportedPred );
}

bool isExtensionSupported( const char* extension, const vector<VkExtensionProperties>& supportedExtensions ){
	const auto isSupportedPred = [extension]( const VkExtensionProperties& prop ) -> bool{
		return std::strcmp( extension, prop.extensionName ) == 0;
	};

	return std::any_of( supportedExtensions.begin(), supportedExtensions.end(), isSupportedPred );
}

vector<const char*> checkInstanceLayerSupport( const vector<const char*>& requestedLayers, const vector<VkLayerProperties>& supportedLayers ){
	vector<const char*> compiledLayerList;

	for( const auto layer : requestedLayers ){
		if(  isLayerSupported( layer, supportedLayers )  ) compiledLayerList.push_back( layer );
		else logger << "WARNING: Requested layer " << layer << " is not supported. It will not be enabled." << std::endl;
	}

	return compiledLayerList;
}

vector<const char*> checkInstanceLayerSupport( const vector<const char*>& optionalLayers ){
	return checkInstanceLayerSupport( optionalLayers, enumerate<VkInstance, VkLayerProperties>() );
}

vector<VkExtensionProperties> getSupportedInstanceExtensions( const vector<const char*>& providingLayers ){
	auto supportedExtensions = enumerate<VkInstance, VkExtensionProperties>();

	for( const auto pl : providingLayers ){
		const auto providedExtensions = enumerate<VkInstance, VkExtensionProperties>( pl );
		supportedExtensions.insert( supportedExtensions.end(), providedExtensions.begin(), providedExtensions.end() );
	}

	return supportedExtensions;
}

vector<VkExtensionProperties> getSupportedDeviceExtensions( const VkPhysicalDevice physDevice, const vector<const char*>& providingLayers ){
	auto supportedExtensions = enumerate<VkExtensionProperties>( physDevice );

	for( const auto pl : providingLayers ){
		const auto providedExtensions = enumerate<VkExtensionProperties>( physDevice, pl );
		supportedExtensions.insert( supportedExtensions.end(), providedExtensions.begin(), providedExtensions.end() );
	}

	return supportedExtensions;
}

bool checkExtensionSupport( const vector<const char*>& extensions, const vector<VkExtensionProperties>& supportedExtensions ){
	bool allSupported = true;

	for( const auto extension : extensions ){
		if(  !isExtensionSupported( extension, supportedExtensions )  ){
			allSupported = false;
			logger << "WARNING: Requested extension " << extension << " is not supported. Trying to enable it will likely fail." << std::endl;
		}
	}

	return allSupported;
}

bool checkDeviceExtensionSupport( const VkPhysicalDevice physDevice, const vector<const char*>& extensions, const vector<const char*>& providingLayers ){
	return checkExtensionSupport(  extensions, getSupportedDeviceExtensions( physDevice, providingLayers )  );
}

VkInstance initInstance( const vector<const char*>& layers, const vector<const char*>& extensions ){
	const VkApplicationInfo appInfo = {
		VK_STRUCTURE_TYPE_APPLICATION_INFO,
		nullptr, // pNext
		VulkanConfig::appName, // Nice to meetcha, and what's your name driver?
		0, // app version
		nullptr, // engine name
		0, // engine version
		VK_API_VERSION_1_0 // this app is written against the Vulkan 1.0 spec
	};

#if VULKAN_VALIDATION
	// in effect during vkCreateInstance and vkDestroyInstance duration (because callback object cannot be created without instance)
	const VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo{
		VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
		nullptr, // pNext
		translateFlags( VulkanConfig::debugSeverity, VulkanConfig::debugType ),
		::genericDebugReportCallback,
		nullptr // pUserData
	};

	const VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = {
		VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		nullptr, // pNext
		0, // flags
		VulkanConfig::debugSeverity,
		VulkanConfig::debugType,
		::genericDebugUtilsCallback,
		nullptr // pUserData
	};

	bool debugUtils = std::find_if( extensions.begin(), extensions.end(), [](const char* e){ return std::strcmp( e, VK_EXT_DEBUG_UTILS_EXTENSION_NAME ) == 0; } ) != extensions.end();
	bool debugReport = std::find_if( extensions.begin(), extensions.end(), [](const char* e){ return std::strcmp( e, VK_EXT_DEBUG_REPORT_EXTENSION_NAME ) == 0; } ) != extensions.end();
	if( !debugUtils && !debugReport ) throw "VULKAN_VALIDATION is enabled but neither VK_EXT_debug_utils nor VK_EXT_debug_report extension is being enabled!";
	const void* debugpNext = debugUtils ? (void*)&debugUtilsCreateInfo : (void*)&debugReportCreateInfo;
#endif

	const VkInstanceCreateInfo instanceInfo{
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#if VULKAN_VALIDATION
		debugpNext,
#else
		nullptr, // pNext
#endif
#ifdef __APPLE__
		0 | VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR, // flags - reserved for future use
#else
    0, // default to no flags for conformant vulkan platforms
#endif
		&appInfo,
		static_cast<uint32_t>( layers.size() ),
		layers.data(),
		static_cast<uint32_t>( extensions.size() ),
		extensions.data()
	};

	VkInstance instance;
	const VkResult errorCode = vkCreateInstance( &instanceInfo, nullptr, &instance ); RESULT_HANDLER( errorCode, "vkCreateInstance" );

	loadInstanceExtensionsCommands( instance, extensions );

	return instance;
}

void killInstance( const VkInstance instance ){
	unloadInstanceExtensionsCommands( instance );

	vkDestroyInstance( instance, nullptr );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool isPresentationSupported( const VkPhysicalDevice physDevice, const uint32_t queueFamily, const VkSurfaceKHR surface ){
	VkBool32 supported;
	const VkResult errorCode = vkGetPhysicalDeviceSurfaceSupportKHR( physDevice, queueFamily, surface, &supported ); RESULT_HANDLER( errorCode, "vkGetPhysicalDeviceSurfaceSupportKHR" );

	return supported == VK_TRUE;
}

bool isPresentationSupported( const VkPhysicalDevice physDevice, const VkSurfaceKHR surface ){
	uint32_t qfCount;
	vkGetPhysicalDeviceQueueFamilyProperties( physDevice, &qfCount, nullptr );

	for( uint32_t qf = 0; qf < qfCount; ++qf ){
		if(  isPresentationSupported( physDevice, qf, surface )  ) return true;
	}

	return false;
}

VkPhysicalDevice getPhysicalDevice( const VkInstance instance, const VkSurfaceKHR surface ){
	vector<VkPhysicalDevice> devices = enumerate<VkPhysicalDevice>( instance );

	if( surface ){
		for( auto it = devices.begin(); it != devices.end(); ){
			const auto& pd = *it;

			if(  !isPresentationSupported( pd, surface )  ) it = devices.erase( it );
			else ++it;
		}
	}

	if( devices.empty() ) throw string("ERROR: No Physical Devices (GPUs) ") + (surface ? "with presentation support " : "") + "detected!";
	else if( devices.size() == 1 ){
		return devices[0];
	}
	else{
		for( const auto pd : devices ){
			const VkPhysicalDeviceProperties pdp = getPhysicalDeviceProperties( pd );

			if( pdp.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ){
#if VULKAN_VALIDATION
				vkDebugReportMessageEXT(
					instance, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, handleToUint64(instance), __LINE__, 
					1, u8"application", u8"More than one Physical Devices (GPU) found. Choosing the first dedicated one."
				);
#endif

				return pd;
			}
		}

#if VULKAN_VALIDATION
		vkDebugReportMessageEXT(
			instance, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, handleToUint64(instance), __LINE__, 
			1, u8"application", u8"More than one Physical Devices (GPU) found. Just choosing the first one."
		);
#endif

		return devices[0];
	}
}

VkPhysicalDeviceProperties getPhysicalDeviceProperties( VkPhysicalDevice physicalDevice ){
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties( physicalDevice, &properties );
	return properties;
}

VkPhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties( VkPhysicalDevice physicalDevice ){
	VkPhysicalDeviceMemoryProperties memoryInfo;
	vkGetPhysicalDeviceMemoryProperties( physicalDevice, &memoryInfo );
	return memoryInfo;
}

vector<VkQueueFamilyProperties> getQueueFamilyProperties( VkPhysicalDevice device ){
	uint32_t queueFamiliesCount;
	vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamiliesCount, nullptr );

	vector<VkQueueFamilyProperties> queueFamilies( queueFamiliesCount );
	vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamiliesCount, queueFamilies.data() );

	return queueFamilies;
}

std::pair<uint32_t, uint32_t> getQueueFamilies( const VkPhysicalDevice physDevice, const VkSurfaceKHR surface ){
	constexpr uint32_t notFound = VK_QUEUE_FAMILY_IGNORED;
	const auto qfps = getQueueFamilyProperties( physDevice );
	const auto findQueueFamilyThat = [&qfps, notFound](std::function<bool (const VkQueueFamilyProperties&, const uint32_t)> predicate) -> uint32_t{
		for( uint32_t qf = 0; qf < qfps.size(); ++qf ) if( predicate(qfps[qf], qf) ) return qf;
		return notFound;
	};

	const auto isGraphics = [](const VkQueueFamilyProperties& props, const uint32_t = 0){
		return props.queueFlags & VK_QUEUE_GRAPHICS_BIT;
	};
	const auto isPresent = [=](const VkQueueFamilyProperties&, const uint32_t queueFamily){
		return isPresentationSupported( physDevice, queueFamily, surface );
	};
	const auto isFusedGraphicsAndPresent = [=](const VkQueueFamilyProperties& props, const uint32_t queueFamily){
		return isGraphics( props ) && isPresent( props, queueFamily );
	};

	uint32_t graphicsQueueFamily = notFound;
	uint32_t presentQueueFamily = notFound;
	if( VulkanConfig::forceSeparatePresentQueue ){
		graphicsQueueFamily = findQueueFamilyThat( isGraphics );

		const auto isSeparatePresent = [graphicsQueueFamily, isPresent](const VkQueueFamilyProperties& props, const uint32_t queueFamily){
			return queueFamily != graphicsQueueFamily && isPresent( props, queueFamily );
		};
		presentQueueFamily = findQueueFamilyThat( isSeparatePresent );
	}
	else{
		graphicsQueueFamily = presentQueueFamily = findQueueFamilyThat( isFusedGraphicsAndPresent );
		if( graphicsQueueFamily == notFound || presentQueueFamily == notFound ){
			graphicsQueueFamily = findQueueFamilyThat( isGraphics );
			presentQueueFamily = findQueueFamilyThat( isPresent );
		}
	}

	if( graphicsQueueFamily == notFound ) throw "Cannot find a graphics queue family!";
	if( presentQueueFamily == notFound ) throw "Cannot find a presentation queue family!";

	return std::make_pair( graphicsQueueFamily, presentQueueFamily );
}

VkDevice initDevice(
	const VkPhysicalDevice physDevice,
	const VkPhysicalDeviceFeatures& features,
	const uint32_t graphicsQueueFamily,
	const uint32_t presentQueueFamily,
	const vector<const char*>& layers,
	const vector<const char*>& extensions
){
	checkDeviceExtensionSupport( physDevice, extensions, layers );

	const float priority[] = {1.0f};

	vector<VkDeviceQueueCreateInfo> queues = {
		{
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			nullptr, // pNext
			0, // flags
			graphicsQueueFamily,
			1, // queue count
			priority
		}
	};

	if( presentQueueFamily != graphicsQueueFamily ){
		queues.push_back({
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			nullptr, // pNext
			0, // flags
			presentQueueFamily,
			1, // queue count
			priority
		});
	}

	const VkDeviceCreateInfo deviceInfo{
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		nullptr, // pNext
		0, // flags
		static_cast<uint32_t>( queues.size() ),
		queues.data(),
		static_cast<uint32_t>( layers.size() ),
		layers.data(),
		static_cast<uint32_t>( extensions.size() ),
		extensions.data(),
		&features
	};


	VkDevice device;
	const VkResult errorCode = vkCreateDevice( physDevice, &deviceInfo, nullptr, &device ); RESULT_HANDLER( errorCode, "vkCreateDevice" );

	loadDeviceExtensionsCommands( device, extensions );

	return device;
}

void killDevice( const VkDevice device ){
	unloadDeviceExtensionsCommands( device );

	vkDestroyDevice( device, nullptr );
}

VkQueue getQueue( const VkDevice device, const uint32_t queueFamily, const uint32_t queueIndex ){
	VkQueue queue;
	vkGetDeviceQueue( device, queueFamily, queueIndex, &queue );

	return queue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//template< ResourceType resourceType, class T >
//VkMemoryRequirements getMemoryRequirements( VkDevice device, T resource );
//
//template<>
//VkMemoryRequirements getMemoryRequirements< ResourceType::Buffer >( VkDevice device, VkBuffer buffer ){
//	VkMemoryRequirements memoryRequirements;
//	vkGetBufferMemoryRequirements( device, buffer, &memoryRequirements );
//
//	return memoryRequirements;
//}
//
//template<>
//VkMemoryRequirements getMemoryRequirements< ResourceType::Image >( VkDevice device, VkImage image ){
//	VkMemoryRequirements memoryRequirements;
//	vkGetImageMemoryRequirements( device, image, &memoryRequirements );
//
//	return memoryRequirements;
//}
//
//template< ResourceType resourceType, class T >
//void bindMemory( VkDevice device, T buffer, VkDeviceMemory memory, VkDeviceSize offset );
//
//template<>
//void bindMemory< ResourceType::Buffer >( VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize offset ){
//	VkResult errorCode = vkBindBufferMemory( device, buffer, memory, offset ); RESULT_HANDLER( errorCode, "vkBindBufferMemory" );
//}
//
//template<>
//void bindMemory< ResourceType::Image >( VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize offset ){
//	VkResult errorCode = vkBindImageMemory( device, image, memory, offset ); RESULT_HANDLER( errorCode, "vkBindImageMemory" );
//}

//template< ResourceType resourceType, class T >
//VkDeviceMemory initMemory(
//	VkDevice device,
//	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties,
//	T resource,
//	const std::vector<VkMemoryPropertyFlags>& memoryTypePriority
//){
//	const VkMemoryRequirements memoryRequirements = getMemoryRequirements<resourceType>( device, resource );
//
//	const auto indexToBit = []( const uint32_t index ){ return 0x1 << index; };
//
//	const uint32_t memoryTypeNotFound = UINT32_MAX;
//	uint32_t memoryType = memoryTypeNotFound;
//	for( const auto desiredMemoryType : memoryTypePriority ){
//		const uint32_t maxMemoryTypeCount = 32;
//		for( uint32_t i = 0; memoryType == memoryTypeNotFound && i < maxMemoryTypeCount; ++i ){
//			if( memoryRequirements.memoryTypeBits & indexToBit(i) ){
//				if( (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryType) == desiredMemoryType ){
//					memoryType = i;
//				}
//			}
//		}
//	}
//
//	if( memoryType == memoryTypeNotFound ) throw "Can't find compatible mappable memory for the resource";
//
//	VkMemoryAllocateInfo memoryInfo{
//		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
//		nullptr, // pNext
//		memoryRequirements.size,
//		memoryType
//	};
//
//	VkDeviceMemory memory;
//	VkResult errorCode = vkAllocateMemory( device, &memoryInfo, nullptr, &memory ); RESULT_HANDLER( errorCode, "vkAllocateMemory" );
//
//	bindMemory<resourceType>( device, resource, memory, 0 /*offset*/ );
//
//	return memory;
//}

void setMemoryData( VkDevice device, VkDeviceMemory memory, void* begin, size_t size ){
	void* data;
	VkResult errorCode = vkMapMemory( device, memory, 0 /*offset*/, VK_WHOLE_SIZE, 0 /*flags - reserved*/, &data ); RESULT_HANDLER( errorCode, "vkMapMemory" );
	memcpy( data, begin, size );
	vkUnmapMemory( device, memory );
}

void killMemory( VkDevice device, VkDeviceMemory memory ){
	vkFreeMemory( device, memory, nullptr );
}


VkBuffer initBuffer( VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage ){
	VkBufferCreateInfo bufferInfo{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr, // pNext
		0, // flags
		size,
		usage,
		VK_SHARING_MODE_EXCLUSIVE,
		0, // queue family count -- ignored for EXCLUSIVE
		nullptr // queue families -- ignored for EXCLUSIVE
	};

	VkBuffer buffer;
	VkResult errorCode = vkCreateBuffer( device, &bufferInfo, nullptr, &buffer ); RESULT_HANDLER( errorCode, "vkCreateBuffer" );
	return buffer;
}

void killBuffer( VkDevice device, VkBuffer buffer ){
	vkDestroyBuffer( device, buffer, nullptr );
}

VkImage initImage( VkDevice device, VkFormat format, uint32_t width, uint32_t height, VkSampleCountFlagBits samples, VkImageUsageFlags usage ){
	VkExtent3D size{
		width,
		height,
		1 // depth
	};

	VkImageCreateInfo ici{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		nullptr, // pNext
		0, // flags
		VK_IMAGE_TYPE_2D,
		format,
		size,
		1, // mipLevels
		1, // arrayLayers
		samples,
		VK_IMAGE_TILING_OPTIMAL,
		usage,
		VK_SHARING_MODE_EXCLUSIVE,
		0, // queueFamilyIndexCount -- ignored for EXCLUSIVE
		nullptr, // pQueueFamilyIndices -- ignored for EXCLUSIVE
		VK_IMAGE_LAYOUT_UNDEFINED
	};

	VkImage image;
	VkResult errorCode = vkCreateImage( device, &ici, nullptr, &image ); RESULT_HANDLER( errorCode, "vkCreateImage" );

	return image;
}

void killImage( VkDevice device, VkImage image ){
	vkDestroyImage( device, image, nullptr );
}

VkImageView initImageView( VkDevice device, VkImage image, VkFormat format ){
	VkImageViewCreateInfo iciv{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr, // pNext
		0, // flags
		image,
		VK_IMAGE_VIEW_TYPE_2D,
		format,
		{ VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
		{
			VK_IMAGE_ASPECT_COLOR_BIT,
			0, // base mip-level
			VK_REMAINING_MIP_LEVELS, // level count
			0, // base array layer
			VK_REMAINING_ARRAY_LAYERS // array layer count
		}
	};

	VkImageView imageView;
	VkResult errorCode = vkCreateImageView( device, &iciv, nullptr, &imageView ); RESULT_HANDLER( errorCode, "vkCreateImageView" );

	return imageView;
}

void killImageView( VkDevice device, VkImageView imageView ){
	vkDestroyImageView( device, imageView, nullptr );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// initSurface is platform dependent

void killSurface( VkInstance instance, VkSurfaceKHR surface ){
	vkDestroySurfaceKHR( instance, surface, nullptr );
}

VkSurfaceFormatKHR getSurfaceFormat( VkPhysicalDevice physicalDevice, VkSurfaceKHR surface ){
	const VkFormat preferredFormat1 = VK_FORMAT_B8G8R8A8_UNORM; 
	const VkFormat preferredFormat2 = VK_FORMAT_B8G8R8A8_SRGB;

	vector<VkSurfaceFormatKHR> formats = enumerate<VkSurfaceFormatKHR>( physicalDevice, surface );

	if( formats.empty() ) throw "No surface formats offered by Vulkan!";

	if( formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED ){
		formats[0].format = preferredFormat1;
	}

	VkSurfaceFormatKHR chosenFormat1 = {VK_FORMAT_UNDEFINED};
	VkSurfaceFormatKHR chosenFormat2 = {VK_FORMAT_UNDEFINED};

	for( auto f : formats ){
		if( f.format == preferredFormat1 ){
			chosenFormat1 = f;
			break;
		}

		if( f.format == preferredFormat2 ){
			chosenFormat2 = f;
		}
	}

	if( chosenFormat1.format ) return chosenFormat1;
	else if( chosenFormat2.format ) return chosenFormat2;
	else return formats[0];
}

VkSurfaceCapabilitiesKHR getSurfaceCapabilities( VkPhysicalDevice physicalDevice, VkSurfaceKHR surface ){
	VkSurfaceCapabilitiesKHR capabilities;
	VkResult errorCode = vkGetPhysicalDeviceSurfaceCapabilitiesKHR( physicalDevice, surface, &capabilities ); RESULT_HANDLER( errorCode, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR" );

	return capabilities;
}

int selectedMode = 0;

TODO( "Could use debug_report instead of log" )
VkPresentModeKHR getSurfacePresentMode( VkPhysicalDevice physicalDevice, VkSurfaceKHR surface ){
	vector<VkPresentModeKHR> modes = enumerate<VkPresentModeKHR>( physicalDevice, surface );

	for( auto m : modes ){
		if( m == VulkanConfig::presentMode ){
			if( selectedMode != 0 ){
				logger << "INFO: Your preferred present mode became supported. Switching to it.\n";
			}

			selectedMode = 0;
			return m;
		}
	}

	for( auto m : modes ){
		if( m == VK_PRESENT_MODE_FIFO_KHR ){
			if( selectedMode != 1 ){
				logger << "WARNING: Your preferred present mode is not supported. Switching to VK_PRESENT_MODE_FIFO_KHR.\n";
			}

			selectedMode = 1;
			return m;
		}
	}

	TODO( "Workaround for bad (Intel Linux Mesa) drivers" )
	if( modes.empty() ) throw "Bugged driver reports no supported present modes.";
	else{
		if( selectedMode != 2 ){
			logger << "WARNING: Bugged drivers. VK_PRESENT_MODE_FIFO_KHR not supported. Switching to whatever is.\n";
		}

		selectedMode = 2;
		return modes[0];
	}
}

VkSwapchainKHR initSwapchain(
	VkPhysicalDevice physicalDevice,
	VkDevice device,
	VkSurfaceKHR surface,
	VkSurfaceFormatKHR surfaceFormat,
	VkSurfaceCapabilitiesKHR capabilities,
	uint32_t graphicsQueueFamily,
	uint32_t presentQueueFamily,
	VkSwapchainKHR oldSwapchain
){
	// we don't care as we are always setting alpha to 1.0
	VkCompositeAlphaFlagBitsKHR compositeAlphaFlag;
	if( capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR ) compositeAlphaFlag = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	else if( capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR ) compositeAlphaFlag = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
	else if( capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR ) compositeAlphaFlag = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
	else if( capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR ) compositeAlphaFlag = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	else throw "Unknown composite alpha reported.";

	// minImageCount + 1 seems a sensible default. It means 2 images should always be readily available without blocking. May lead to memory waste though if we care about that.
	uint32_t myMinImageCount = capabilities.minImageCount + 1;
	if( capabilities.maxImageCount ) myMinImageCount = std::min<uint32_t>( myMinImageCount, capabilities.maxImageCount );

	const std::vector<uint32_t> queueFamilies = { graphicsQueueFamily, presentQueueFamily };
	VkSwapchainCreateInfoKHR swapchainInfo{
		VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		nullptr, // pNext for extensions use
		0, // flags - reserved for future use
		surface,
		myMinImageCount, // minImageCount
		surfaceFormat.format,
		surfaceFormat.colorSpace,
		capabilities.currentExtent,
		1,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // VkImage usage flags
		// It should be fine to just use CONCURRENT in the off chance we encounter the elusive GPU with separate present queue
		graphicsQueueFamily == presentQueueFamily ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
		static_cast<uint32_t>( queueFamilies.size() ),
		queueFamilies.data(),
		capabilities.currentTransform,
		compositeAlphaFlag,
		getSurfacePresentMode( physicalDevice, surface ),
		VK_TRUE, // clipped
		oldSwapchain
	};

	VkSwapchainKHR swapchain;
	VkResult errorCode = vkCreateSwapchainKHR( device, &swapchainInfo, nullptr, &swapchain ); RESULT_HANDLER( errorCode, "vkCreateSwapchainKHR" );

	return swapchain;
}

void killSwapchain( VkDevice device, VkSwapchainKHR swapchain ){
	vkDestroySwapchainKHR( device, swapchain, nullptr );
}

uint32_t getNextImageIndex( VkDevice device, VkSwapchainKHR swapchain, VkSemaphore imageReadyS ){
	uint32_t nextImageIndex;
	VkResult errorCode = vkAcquireNextImageKHR(
		device,
		swapchain,
		UINT64_MAX /* no timeout */,
		imageReadyS,
		VK_NULL_HANDLE,
		&nextImageIndex
	); RESULT_HANDLER( errorCode, "vkAcquireNextImageKHR" );

	return nextImageIndex;
}

vector<VkImageView> initSwapchainImageViews( VkDevice device, vector<VkImage> images, VkFormat format ){
	vector<VkImageView> imageViews;

	for( auto image : images ){
		VkImageView imageView = initImageView( device, image, format );

		imageViews.push_back( imageView );
	}

	return imageViews;
}

void killSwapchainImageViews( VkDevice device, vector<VkImageView>& imageViews ){
	for( auto imageView : imageViews ) vkDestroyImageView( device, imageView, nullptr );
	imageViews.clear();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VkRenderPass initRenderPass( VkDevice device, VkSurfaceFormatKHR surfaceFormat ){
	VkAttachmentDescription colorAtachment{
		0, // flags
		surfaceFormat.format,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR, // color + depth
		VK_ATTACHMENT_STORE_OP_STORE, // color + depth
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, // stencil
		VK_ATTACHMENT_STORE_OP_DONT_CARE, // stencil
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference colorReference{
		0, // attachment
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass{
		0, // flags - reserved for future use
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		0, // input attachment count
		nullptr, // input attachments
		1, // color attachment count
		&colorReference, // color attachments
		nullptr, // resolve attachments
		nullptr, // depth stencil attachment
		0, // preserve attachment count
		nullptr // preserve attachments
	};

	VkSubpassDependency srcDependency{
		VK_SUBPASS_EXTERNAL, // srcSubpass
		0, // dstSubpass
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStageMask
		0, // srcAccessMask
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // dstAccessMask
		VK_DEPENDENCY_BY_REGION_BIT, // dependencyFlags
	};

	// implicitly defined dependency would cover this, but let's replace it with this explicitly defined dependency!
	VkSubpassDependency dstDependency{
		0, // srcSubpass
		VK_SUBPASS_EXTERNAL, // dstSubpass
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // dstStageMask
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // srcAccessMask
		0, // dstAccessMask
		VK_DEPENDENCY_BY_REGION_BIT, // dependencyFlags
	};

	VkSubpassDependency dependencies[] = {srcDependency, dstDependency};

	VkRenderPassCreateInfo renderPassInfo{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr, // pNext
		0, // flags - reserved for future use
		1, // attachment count
		&colorAtachment, // attachments
		1, // subpass count
		&subpass, // subpasses
		2, // dependency count
		dependencies // dependencies
	};

	VkRenderPass renderPass;
	VkResult errorCode = vkCreateRenderPass( device, &renderPassInfo, nullptr, &renderPass ); RESULT_HANDLER( errorCode, "vkCreateRenderPass" );

	return renderPass;
}

void killRenderPass( VkDevice device, VkRenderPass renderPass ){
	vkDestroyRenderPass( device, renderPass, nullptr );
}

vector<VkFramebuffer> initFramebuffers(
	VkDevice device,
	VkRenderPass renderPass,
	vector<VkImageView> imageViews,
	uint32_t width, uint32_t height
){
	vector<VkFramebuffer> framebuffers;

	for( auto imageView : imageViews ){
		VkFramebufferCreateInfo framebufferInfo{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			nullptr, // pNext
			0, // flags - reserved for future use
			renderPass,
			1, // ImageView count
			&imageView,
			width, // width
			height, // height
			1 // layers
		};

		VkFramebuffer framebuffer;
		VkResult errorCode = vkCreateFramebuffer( device, &framebufferInfo, nullptr, &framebuffer ); RESULT_HANDLER( errorCode, "vkCreateFramebuffer" );
		framebuffers.push_back( framebuffer );
	}

	return framebuffers;
}

void killFramebuffers( VkDevice device, vector<VkFramebuffer>& framebuffers ){
	for( auto framebuffer : framebuffers ) vkDestroyFramebuffer( device, framebuffer, nullptr );
	framebuffers.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//template<typename Type = uint8_t>
//vector<Type> loadBinaryFile( string filename ){
//	using std::ifstream;
//	using std::istreambuf_iterator;
//
//	vector<Type> data;
//
//	try{
//		ifstream ifs;
//		ifs.exceptions( ifs.failbit | ifs.badbit | ifs.eofbit );
//		ifs.open( filename, ifs.in | ifs.binary | ifs.ate );
//
//		const auto fileSize = static_cast<size_t>( ifs.tellg() );
//
//		if( fileSize > 0 && (fileSize % sizeof(Type) == 0) ){
//			ifs.seekg( ifs.beg );
//			data.resize( fileSize / sizeof(Type) );
//			ifs.read( reinterpret_cast<char*>(data.data()), fileSize );
//		}
//	}
//	catch( ... ){
//		data.clear();
//	}
//
//	return data;
//}

VkShaderModule initShaderModule( VkDevice device, string filename ){
	const auto shaderCode = loadBinaryFile<uint32_t>( filename );
	if( shaderCode.empty() ) throw "SPIR-V shader file " + filename + " is invalid or read failed!";
	return initShaderModule( device, shaderCode );
}

VkShaderModule initShaderModule( VkDevice device, const vector<uint32_t>& shaderCode ){
	VkShaderModuleCreateInfo shaderModuleInfo{
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		nullptr, // pNext
		0, // flags - reserved for future use
		shaderCode.size() * sizeof(uint32_t),
		shaderCode.data()
	};

	VkShaderModule shaderModule;
	VkResult errorCode = vkCreateShaderModule( device, &shaderModuleInfo, nullptr, &shaderModule ); RESULT_HANDLER( errorCode, "vkCreateShaderModule" );

	return shaderModule;
}

void killShaderModule( VkDevice device, VkShaderModule shaderModule ){
	vkDestroyShaderModule( device, shaderModule, nullptr );
}

VkPipelineLayout initPipelineLayout( VkDevice device ){
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		nullptr, // pNext
		0, // flags - reserved for future use
		0, // descriptorSetLayout count
		nullptr,
		0, // push constant range count
		nullptr // push constant ranges
	};

	VkPipelineLayout pipelineLayout;
	VkResult errorCode = vkCreatePipelineLayout( device, &pipelineLayoutInfo, nullptr, &pipelineLayout ); RESULT_HANDLER( errorCode, "vkCreatePipelineLayout" );

	return pipelineLayout;
}

void killPipelineLayout( VkDevice device, VkPipelineLayout pipelineLayout ){
	vkDestroyPipelineLayout( device, pipelineLayout, nullptr );
}

VkPipeline initPipeline(
	VkDevice device,
	VkPhysicalDeviceLimits limits,
	VkPipelineLayout pipelineLayout,
	VkRenderPass renderPass,
	VkShaderModule vertexShader,
	VkShaderModule fragmentShader,
	const uint32_t vertexBufferBinding,
	uint32_t width, uint32_t height
){/*
	const VkPipelineShaderStageCreateInfo vertexShaderStage{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		nullptr, // pNext
		0, // flags - reserved for future use
		VK_SHADER_STAGE_VERTEX_BIT,
		vertexShader,
		u8"main",
		nullptr // SpecializationInfo - constants pushed to shader on pipeline creation time
	};

	const VkPipelineShaderStageCreateInfo fragmentShaderStage{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		nullptr, // pNext
		0, // flags - reserved for future use
		VK_SHADER_STAGE_FRAGMENT_BIT,
		fragmentShader,
		u8"main",
		nullptr // SpecializationInfo - constants pushed to shader on pipeline creation time
	};*/

	VkPipelineShaderStageCreateInfo shaderStageStates[] = { 
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			nullptr, // pNext
			0, // flags - reserved for future use
			VK_SHADER_STAGE_VERTEX_BIT,
			vertexShader,
			u8"main",
			nullptr // SpecializationInfo - constants pushed to shader on pipeline creation time
		}, 
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			nullptr, // pNext
			0, // flags - reserved for future use
			VK_SHADER_STAGE_FRAGMENT_BIT,
			fragmentShader,
			u8"main",
			nullptr // SpecializationInfo - constants pushed to shader on pipeline creation time
		}
	};

	const uint32_t vertexBufferStride = sizeof( Vertex2D_ColorF_pack );
	if( vertexBufferBinding > limits.maxVertexInputBindings ){
		throw string("Implementation does not allow enough input bindings. Needed: ")
		    + to_string( vertexBufferBinding ) + string(", max: ")
		    + to_string( limits.maxVertexInputBindings );
	}
	if( vertexBufferStride > limits.maxVertexInputBindingStride ){
		throw string("Implementation does not allow big enough vertex buffer stride: ")
		    + to_string( vertexBufferStride ) 
		    + string(", max: ")
		    + to_string( limits.maxVertexInputBindingStride );
	}

	VkVertexInputBindingDescription vertexInputBindingDescription{
		vertexBufferBinding,
		sizeof( Vertex2D_ColorF_pack ), // stride in bytes
		VK_VERTEX_INPUT_RATE_VERTEX
	};

	vector<VkVertexInputBindingDescription> inputBindingDescriptions = { vertexInputBindingDescription };
	if( inputBindingDescriptions.size() > limits.maxVertexInputBindings ){
		throw "Implementation does not allow enough input bindings.";
	}

	const uint32_t positionLocation = 0;
	const uint32_t colorLocation = 1;

	if( colorLocation >= limits.maxVertexInputAttributes ){
		throw "Implementation does not allow enough input attributes.";
	}
	if( offsetof( Vertex2D_ColorF_pack, color ) > limits.maxVertexInputAttributeOffset ){
		throw "Implementation does not allow sufficient attribute offset.";
	}

	VkVertexInputAttributeDescription positionInputAttributeDescription{
		positionLocation,
		vertexBufferBinding,
		VK_FORMAT_R32G32_SFLOAT,
		offsetof( Vertex2D_ColorF_pack, position ) // offset in bytes
	};

	VkVertexInputAttributeDescription colorInputAttributeDescription{
		colorLocation,
		vertexBufferBinding,
		VK_FORMAT_R32G32B32_SFLOAT,
		offsetof( Vertex2D_ColorF_pack, color ) // offset in bytes
	};

	vector<VkVertexInputAttributeDescription> inputAttributeDescriptions = {
		positionInputAttributeDescription,
		colorInputAttributeDescription
	};

	VkPipelineVertexInputStateCreateInfo vertexInputState{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr, // pNext
		0, // flags - reserved for future use
		static_cast<uint32_t>( inputBindingDescriptions.size() ),
		inputBindingDescriptions.data(),
		static_cast<uint32_t>( inputAttributeDescriptions.size() ),
		inputAttributeDescriptions.data()
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		nullptr, // pNext
		0, // flags - reserved for future use
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_FALSE // primitive restart
	};

	VkViewport viewport{
		0.0f, // x
		0.0f, // y
		static_cast<float>( width ? width : 1 ),
		static_cast<float>( height ? height : 1 ),
		0.0f, // min depth
		1.0f // max depth
	};

	VkRect2D scissor{
		{0, 0}, // offset
		{width, height}
	};

	VkPipelineViewportStateCreateInfo viewportState{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		nullptr, // pNext
		0, // flags - reserved for future use
		1, // Viewport count
		&viewport,
		1, // scisor count,
		&scissor
	};

	VkPipelineRasterizationStateCreateInfo rasterizationState{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		nullptr, // pNext
		0, // flags - reserved for future use
		VK_FALSE, // depth clamp
		VK_FALSE, // rasterizer discard
		VK_POLYGON_MODE_FILL,
		VK_CULL_MODE_BACK_BIT,
		VK_FRONT_FACE_COUNTER_CLOCKWISE,
		VK_FALSE, // depth bias
		0.0f, // bias constant factor
		0.0f, // bias clamp
		0.0f, // bias slope factor
		1.0f // line width
	};

	VkPipelineMultisampleStateCreateInfo multisampleState{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		nullptr, // pNext
		0, // flags - reserved for future use
		VK_SAMPLE_COUNT_1_BIT,
		VK_FALSE, // no sample shading
		0.0f, // min sample shading - ignored if disabled
		nullptr, // sample mask
		VK_FALSE, // alphaToCoverage
		VK_FALSE // alphaToOne
	};

	VkPipelineColorBlendAttachmentState blendAttachmentState{
		VK_FALSE, // blending enabled?
		VK_BLEND_FACTOR_ZERO, // src blend factor -ignored?
		VK_BLEND_FACTOR_ZERO, // dst blend factor
		VK_BLEND_OP_ADD, // blend op
		VK_BLEND_FACTOR_ZERO, // src alpha blend factor
		VK_BLEND_FACTOR_ZERO, // dst alpha blend factor
		VK_BLEND_OP_ADD, // alpha blend op
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT // color write mask
	};

	VkPipelineColorBlendStateCreateInfo colorBlendState{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		nullptr, // pNext
		0, // flags - reserved for future use
		VK_FALSE, // logic ops
		VK_LOGIC_OP_COPY,
		1, // attachment count - must be same as color attachment count in renderpass subpass!
		&blendAttachmentState,
		{0.0f, 0.0f, 0.0f, 0.0f} // blend constants
	};

	VkGraphicsPipelineCreateInfo pipelineInfo{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		nullptr, // pNext
		0, // flags - e.g. disable optimization
		2, // shader stages count - vertex and fragment
		shaderStageStates,
		&vertexInputState,
		&inputAssemblyState,
		nullptr, // tesselation
		&viewportState,
		&rasterizationState,
		&multisampleState,
		nullptr, // depth stencil
		&colorBlendState,
		nullptr, // dynamic state
		pipelineLayout,
		renderPass,
		0, // subpass index in renderpass
		VK_NULL_HANDLE, // base pipeline
		-1 // base pipeline index
	};

	VkPipeline pipeline;
	VkResult errorCode = vkCreateGraphicsPipelines(
		device,
		VK_NULL_HANDLE /* pipeline cache */,
		1 /* info count */,
		&pipelineInfo,
		nullptr,
		&pipeline
	); RESULT_HANDLER( errorCode, "vkCreateGraphicsPipelines" );
	return pipeline;
}

void killPipeline( VkDevice device, VkPipeline pipeline ){
	vkDestroyPipeline( device, pipeline, nullptr );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setVertexData( VkDevice device, VkDeviceMemory memory, vector<Vertex2D_ColorF_pack> vertices ){
	TODO( "Should be in Device Local memory instead" )
	setMemoryData(  device, memory, vertices.data(), sizeof( decltype(vertices)::value_type ) * vertices.size()  );
}

VkSemaphore initSemaphore( VkDevice device ){
	const VkSemaphoreCreateInfo semaphoreInfo{
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		nullptr, // pNext
		0 // flags - reserved for future use
	};

	VkSemaphore semaphore;
	VkResult errorCode = vkCreateSemaphore( device, &semaphoreInfo, nullptr, &semaphore ); RESULT_HANDLER( errorCode, "vkCreateSemaphore" );
	return semaphore;
}

vector<VkSemaphore> initSemaphores( VkDevice device, size_t count ){
	vector<VkSemaphore> semaphores;
	std::generate_n(  std::back_inserter( semaphores ), count, [device]{ return initSemaphore( device ); }  );
	return semaphores;
}

void killSemaphore( VkDevice device, VkSemaphore semaphore ){
	vkDestroySemaphore( device, semaphore, nullptr );
}

void killSemaphores( VkDevice device, vector<VkSemaphore>& semaphores ){
	for( const auto s : semaphores ) killSemaphore( device, s );
	semaphores.clear();
}

VkCommandPool initCommandPool( VkDevice device, const uint32_t queueFamily ){
	const VkCommandPoolCreateInfo commandPoolInfo{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		nullptr, // pNext
		0, // flags
		queueFamily
	};

	VkCommandPool commandPool;
	VkResult errorCode = vkCreateCommandPool( device, &commandPoolInfo, nullptr, &commandPool ); RESULT_HANDLER( errorCode, "vkCreateCommandPool" );
	return commandPool;
}

void killCommandPool( VkDevice device, VkCommandPool commandPool ){
	vkDestroyCommandPool( device, commandPool, nullptr );
}

VkFence initFence( const VkDevice device, const VkFenceCreateFlags flags = 0 ){
	const VkFenceCreateInfo fci{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		nullptr, // pNext
		flags
	};

	VkFence fence;
	VkResult errorCode = vkCreateFence( device, &fci, nullptr, &fence ); RESULT_HANDLER( errorCode, "vkCreateFence" );
	return fence;
}

void killFence( const VkDevice device, const VkFence fence ){
	vkDestroyFence( device, fence, nullptr );
}

vector<VkFence> initFences( const VkDevice device, const size_t count, const VkFenceCreateFlags flags ){
	vector<VkFence> fences;
	std::generate_n(  std::back_inserter( fences ), count, [=]{return initFence( device, flags );}  );
	return fences;
}

void killFences( const VkDevice device, vector<VkFence>& fences ){
	for( const auto f : fences ) killFence( device, f );
	fences.clear();
}

void acquireCommandBuffers( VkDevice device, VkCommandPool commandPool, uint32_t count, vector<VkCommandBuffer>& commandBuffers ){
	const auto oldSize = static_cast<uint32_t>( commandBuffers.size() );

	if( count > oldSize ){
		VkCommandBufferAllocateInfo commandBufferInfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			nullptr, // pNext
			commandPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			count - oldSize // count
		};

		commandBuffers.resize( count );
		VkResult errorCode = vkAllocateCommandBuffers( device, &commandBufferInfo, &commandBuffers[oldSize] ); RESULT_HANDLER( errorCode, "vkAllocateCommandBuffers" );
	}

	if( count < oldSize ) {
		vkFreeCommandBuffers( device, commandPool, oldSize - count, &commandBuffers[count] );
		commandBuffers.resize( count );
	}
}

void beginCommandBuffer( VkCommandBuffer commandBuffer ){
	VkCommandBufferBeginInfo commandBufferInfo{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr, // pNext
		// same buffer can be re-executed before it finishes from last submit
		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, // flags
		nullptr // inheritance
	};

	VkResult errorCode = vkBeginCommandBuffer( commandBuffer, &commandBufferInfo ); RESULT_HANDLER( errorCode, "vkBeginCommandBuffer" );
}

void endCommandBuffer( VkCommandBuffer commandBuffer ){
	VkResult errorCode = vkEndCommandBuffer( commandBuffer ); RESULT_HANDLER( errorCode, "vkEndCommandBuffer" );
}


void recordBeginRenderPass(
	VkCommandBuffer commandBuffer,
	VkRenderPass renderPass,
	VkFramebuffer framebuffer,
	VkClearValue clearValue,
	uint32_t width, uint32_t height
){
	VkRenderPassBeginInfo renderPassInfo{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		nullptr, // pNext
		renderPass,
		framebuffer,
		{{0,0}, {width,height}}, //render area - offset plus extent
		1, // clear value count
		&clearValue
	};

	vkCmdBeginRenderPass( commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
}

void recordEndRenderPass( VkCommandBuffer commandBuffer ){
	vkCmdEndRenderPass( commandBuffer );
}

void recordBindPipeline( VkCommandBuffer commandBuffer, VkPipeline pipeline ){
	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
}

void recordBindVertexBuffer( VkCommandBuffer commandBuffer, const uint32_t vertexBufferBinding, VkBuffer vertexBuffer ){
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers( commandBuffer, vertexBufferBinding, 1 /*binding count*/, &vertexBuffer, offsets );
}

void recordDraw( VkCommandBuffer commandBuffer, const uint32_t vertexCount ){
	vkCmdDraw( commandBuffer, vertexCount, 1 /*instance count*/, 0 /*first vertex*/, 0 /*first instance*/ );
}

void submitToQueue( VkQueue queue, VkCommandBuffer commandBuffer, VkSemaphore imageReadyS, VkSemaphore renderDoneS, VkFence fence ){
	const VkPipelineStageFlags psw = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	const VkSubmitInfo submit{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		nullptr, // pNext
		1, &imageReadyS, // wait semaphores
		&psw, // pipeline stages to wait for semaphore
		1, &commandBuffer,
		1, &renderDoneS // signal semaphores
	};

	const VkResult errorCode = vkQueueSubmit( queue, 1 /*submit count*/, &submit, fence ); RESULT_HANDLER( errorCode, "vkQueueSubmit" );
}

void present( VkQueue queue, VkSwapchainKHR swapchain, uint32_t swapchainImageIndex, VkSemaphore renderDoneS ){
	const VkPresentInfoKHR presentInfo{
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		nullptr, // pNext
		1, &renderDoneS, // wait semaphores
		1, &swapchain, &swapchainImageIndex,
		nullptr // pResults
	};

	const VkResult errorCode = vkQueuePresentKHR( queue, &presentInfo ); RESULT_HANDLER( errorCode, "vkQueuePresentKHR" );
}

// cleanup dangerous semaphore with signal pending from vkAcquireNextImageKHR (tie it to a specific queue)
// https://github.com/KhronosGroup/Vulkan-Docs/issues/1059
void cleanupUnsafeSemaphore( VkQueue queue, VkSemaphore semaphore ){
	VkPipelineStageFlags psw = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	const VkSubmitInfo submit{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		nullptr, // pNext
		1, &semaphore, // wait semaphores
		&psw, // pipeline stages to wait for semaphore
		0, nullptr, // command buffers
		0, nullptr // signal semaphores
	};

	const VkResult errorCode = vkQueueSubmit( queue, 1 /*submit count*/, &submit, VK_NULL_HANDLE ); RESULT_HANDLER( errorCode, "vkQueueSubmit" );
}

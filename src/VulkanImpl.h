#ifndef COMMON_VULKAN_IMPL_H
#define COMMON_VULKAN_IMPL_H

using std::exception;
using std::string;
using std::to_string;
using std::vector;

#include "Vertex.h"
#include "ErrorHandling.h"
#include "Wsi.h"

//  forward declarations
//////////////////////////////////////////////////////////////////////////////////

bool isLayerSupported( const char* layer, const vector<VkLayerProperties>& supportedLayers );
bool isExtensionSupported( const char* extension, const vector<VkExtensionProperties>& supportedExtensions );
// treat layers as optional; app can always run without em -- i.e. return those supported
vector<const char*> checkInstanceLayerSupport( const vector<const char*>& requestedLayers, const vector<VkLayerProperties>& supportedLayers );
vector<VkExtensionProperties> getSupportedInstanceExtensions( const vector<const char*>& providingLayers );
bool checkExtensionSupport( const vector<const char*>& extensions, const vector<VkExtensionProperties>& supportedExtensions );

VkInstance initInstance( const vector<const char*>& layers = {}, const vector<const char*>& extensions = {} );
void killInstance( VkInstance instance );

VkPhysicalDevice getPhysicalDevice( VkInstance instance, VkSurfaceKHR surface = VK_NULL_HANDLE /*seek presentation support if !NULL*/ ); // destroyed with instance
VkPhysicalDeviceProperties getPhysicalDeviceProperties( VkPhysicalDevice physicalDevice );
VkPhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties( VkPhysicalDevice physicalDevice );

std::pair<uint32_t, uint32_t> getQueueFamilies( VkPhysicalDevice physDevice, VkSurfaceKHR surface );
vector<VkQueueFamilyProperties> getQueueFamilyProperties( VkPhysicalDevice device );

VkDevice initDevice(
	VkPhysicalDevice physDevice,
	const VkPhysicalDeviceFeatures& features,
	uint32_t graphicsQueueFamily,
	uint32_t presentQueueFamily,
	const vector<const char*>& layers = {},
	const vector<const char*>& extensions = {}
);
void killDevice( VkDevice device );

VkQueue getQueue( VkDevice device, uint32_t queueFamily, uint32_t queueIndex );


enum class ResourceType{ Buffer, Image };

template< ResourceType resourceType, class T >
VkDeviceMemory initMemory(
	VkDevice device,
	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties,
	T resource,
	const std::vector<VkMemoryPropertyFlags>& memoryTypePriority
);
void setMemoryData( VkDevice device, VkDeviceMemory memory, void* begin, size_t size );
void killMemory( VkDevice device, VkDeviceMemory memory );

VkBuffer initBuffer( VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage );
void killBuffer( VkDevice device, VkBuffer buffer );

VkImage initImage(
	VkDevice device,
	VkFormat format,
	uint32_t width, uint32_t height,
	VkSampleCountFlagBits samples,
	VkImageUsageFlags usage
);
void killImage( VkDevice device, VkImage image );

VkImageView initImageView( VkDevice device, VkImage image, VkFormat format );
void killImageView( VkDevice device, VkImageView imageView );

// initSurface() is platform dependent
void killSurface( VkInstance instance, VkSurfaceKHR surface );

VkSurfaceCapabilitiesKHR getSurfaceCapabilities( VkPhysicalDevice physicalDevice, VkSurfaceKHR surface );
VkSurfaceFormatKHR getSurfaceFormat( VkPhysicalDevice physicalDevice, VkSurfaceKHR surface );

VkSwapchainKHR initSwapchain(
	VkPhysicalDevice physicalDevice,
	VkDevice device,
	VkSurfaceKHR surface,
	VkSurfaceFormatKHR surfaceFormat,
	VkSurfaceCapabilitiesKHR capabilities,
	uint32_t graphicsQueueFamily,
	uint32_t presentQueueFamily,
	VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE
);
void killSwapchain( VkDevice device, VkSwapchainKHR swapchain );

uint32_t getNextImageIndex( VkDevice device, VkSwapchainKHR swapchain, VkSemaphore imageReadyS );

vector<VkImageView> initSwapchainImageViews( VkDevice device, vector<VkImage> images, VkFormat format );
void killSwapchainImageViews( VkDevice device, vector<VkImageView>& imageViews );


VkRenderPass initRenderPass( VkDevice device, VkSurfaceFormatKHR surfaceFormat );
void killRenderPass( VkDevice device, VkRenderPass renderPass );

vector<VkFramebuffer> initFramebuffers(
	VkDevice device,
	VkRenderPass renderPass,
	vector<VkImageView> imageViews,
	uint32_t width, uint32_t height
);
void killFramebuffers( VkDevice device, vector<VkFramebuffer>& framebuffers );


VkShaderModule initShaderModule( VkDevice device, const vector<uint32_t>& shaderCode );
VkShaderModule initShaderModule( VkDevice device, string filename );
void killShaderModule( VkDevice device, VkShaderModule shaderModule );

VkPipelineLayout initPipelineLayout( VkDevice device );
void killPipelineLayout( VkDevice device, VkPipelineLayout pipelineLayout );

VkPipeline initPipeline(
	VkDevice device,
	VkPhysicalDeviceLimits limits,
	VkPipelineLayout pipelineLayout,
	VkRenderPass renderPass,
	VkShaderModule vertexShader,
	VkShaderModule fragmentShader,
	const uint32_t vertexBufferBinding,
	uint32_t width, uint32_t height
);
void killPipeline( VkDevice device, VkPipeline pipeline );


void setVertexData( VkDevice device, VkDeviceMemory memory, vector<Vertex2D_ColorF_pack> vertices );

VkSemaphore initSemaphore( VkDevice device );
vector<VkSemaphore> initSemaphores( VkDevice device, size_t count );
void killSemaphore( VkDevice device, VkSemaphore semaphore );
void killSemaphores( VkDevice device, vector<VkSemaphore>& semaphores );

VkCommandPool initCommandPool( VkDevice device, const uint32_t queueFamily );
void killCommandPool( VkDevice device, VkCommandPool commandPool );

vector<VkFence> initFences( VkDevice device, size_t count, VkFenceCreateFlags flags = 0 );
void killFences( VkDevice device, vector<VkFence>& fences );

void acquireCommandBuffers( VkDevice device, VkCommandPool commandPool, uint32_t count, vector<VkCommandBuffer>& commandBuffers );
void beginCommandBuffer( VkCommandBuffer commandBuffer );
void endCommandBuffer( VkCommandBuffer commandBuffer );

void recordBeginRenderPass(
	VkCommandBuffer commandBuffer,
	VkRenderPass renderPass,
	VkFramebuffer framebuffer,
	VkClearValue clearValue,
	uint32_t width, uint32_t height
);
void recordEndRenderPass( VkCommandBuffer commandBuffer );

void recordBindPipeline( VkCommandBuffer commandBuffer, VkPipeline pipeline );
void recordBindVertexBuffer( VkCommandBuffer commandBuffer, const uint32_t vertexBufferBinding, VkBuffer vertexBuffer );

void recordDraw( VkCommandBuffer commandBuffer, uint32_t vertexCount );

void submitToQueue( VkQueue queue, VkCommandBuffer commandBuffer, VkSemaphore imageReadyS, VkSemaphore renderDoneS, VkFence fence = VK_NULL_HANDLE );
void present( VkQueue queue, VkSwapchainKHR swapchain, uint32_t swapchainImageIndex, VkSemaphore renderDoneS );

// cleanup dangerous semaphore with signal pending from vkAcquireNextImageKHR
void cleanupUnsafeSemaphore( VkQueue queue, VkSemaphore semaphore );

//template implementation
template<typename Type = uint8_t>
inline vector<Type> loadBinaryFile( string filename ){
	using std::ifstream;
	using std::istreambuf_iterator;

	vector<Type> data;

	try{
		ifstream ifs;
		ifs.exceptions( ifs.failbit | ifs.badbit | ifs.eofbit );
		ifs.open( filename, ifs.in | ifs.binary | ifs.ate );

		const auto fileSize = static_cast<size_t>( ifs.tellg() );

		if( fileSize > 0 && (fileSize % sizeof(Type) == 0) ){
			ifs.seekg( ifs.beg );
			data.resize( fileSize / sizeof(Type) );
			ifs.read( reinterpret_cast<char*>(data.data()), fileSize );
		}
	}
	catch( ... ){
		data.clear();
	}

	return data;
}

template< ResourceType resourceType, class T >
inline VkMemoryRequirements getMemoryRequirements( VkDevice device, T resource );

template<>
inline VkMemoryRequirements getMemoryRequirements< ResourceType::Buffer >( VkDevice device, VkBuffer buffer ){
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements( device, buffer, &memoryRequirements );

	return memoryRequirements;
}

template<>
inline VkMemoryRequirements getMemoryRequirements< ResourceType::Image >( VkDevice device, VkImage image ){
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements( device, image, &memoryRequirements );

	return memoryRequirements;
}

template< ResourceType resourceType, class T >
inline void bindMemory( VkDevice device, T buffer, VkDeviceMemory memory, VkDeviceSize offset );

template<>
inline void bindMemory< ResourceType::Buffer >( VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize offset ){
	VkResult errorCode = vkBindBufferMemory( device, buffer, memory, offset ); RESULT_HANDLER( errorCode, "vkBindBufferMemory" );
}

template<>
inline void bindMemory< ResourceType::Image >( VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize offset ){
	VkResult errorCode = vkBindImageMemory( device, image, memory, offset ); RESULT_HANDLER( errorCode, "vkBindImageMemory" );
}

template< ResourceType resourceType, class T >
inline VkDeviceMemory initMemory(
	VkDevice device,
	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties,
	T resource,
	const std::vector<VkMemoryPropertyFlags>& memoryTypePriority
){
	const VkMemoryRequirements memoryRequirements = getMemoryRequirements<resourceType>( device, resource );

	const auto indexToBit = []( const uint32_t index ){ return 0x1 << index; };

	const uint32_t memoryTypeNotFound = UINT32_MAX;
	uint32_t memoryType = memoryTypeNotFound;
	for( const auto desiredMemoryType : memoryTypePriority ){
		const uint32_t maxMemoryTypeCount = 32;
		for( uint32_t i = 0; memoryType == memoryTypeNotFound && i < maxMemoryTypeCount; ++i ){
			if( memoryRequirements.memoryTypeBits & indexToBit(i) ){
				if( (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryType) == desiredMemoryType ){
					memoryType = i;
				}
			}
		}
	}

	if( memoryType == memoryTypeNotFound ) throw "Can't find compatible mappable memory for the resource";

	VkMemoryAllocateInfo memoryInfo{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr, // pNext
		memoryRequirements.size,
		memoryType
	};

	VkDeviceMemory memory;
	VkResult errorCode = vkAllocateMemory( device, &memoryInfo, nullptr, &memory ); RESULT_HANDLER( errorCode, "vkAllocateMemory" );

	bindMemory<resourceType>( device, resource, memory, 0 /*offset*/ );

	return memory;
}

//Kill all vulkan handles
void cleanupVulkan(VkDevice device,
    VkInstance instance,
    vector<VkSemaphore>& renderSs,
    VkPipeline pipeline,
    vector<VkFramebuffer>& framebuffers,
    vector<VkImageView>& imageViews,
    VkSwapchainKHR swapchain,
    vector<VkSemaphore>& imageSs,
    vector<VkFence>& fences,
    VkCommandPool commandPool,
    VkDeviceMemory vertexBufferMemory,
    VkBuffer vertexBuffer,
    VkPipelineLayout pipelineLayout,
    VkShaderModule fragmentShader,
    VkShaderModule vertexShader,
    VkRenderPass renderPass,
    VkSurfaceKHR surface,
    PlatformWindow window
);
#endif //COMMON_VULKAN_IMPL_H

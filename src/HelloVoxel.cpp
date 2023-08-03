// Vulkan hello world triangle rendering demo


// Global header settings
//////////////////////////////////////////////////////////////////////////////////

#include "VulkanEnvironment.h" // first include must be before vulkan.h and platform header


// Includes
//////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#define VK_ENABLE_BETA_EXTENSIONS //Include provisional headers
#include <vulkan/vulkan.h> // also assume core+WSI commands are loaded
static_assert( VK_HEADER_VERSION >= REQUIRED_HEADER_VERSION, "Update your SDK! This app is written against Vulkan header version " STRINGIZE(REQUIRED_HEADER_VERSION) "." );

#include "EnumerateScheme.h"
#include "ErrorHandling.h"
#include "ExtensionLoader.h"
#include "Vertex.h"
#include "Wsi.h"

#include "VulkanConfig.h"
#include "VulkanImpl.h"


using std::exception;
using std::runtime_error;
using std::string;
using std::to_string;
using std::vector;

// main()!
//////////////////////////////////////////////////////////////////////////////////

int helloTriangle() try{
	const uint32_t vertexBufferBinding = 0;

	const float triangleSize = 1.6f;
	const vector<Vertex2D_ColorF_pack> triangle = {
		{ /*rb*/ { { 0.5f * triangleSize,  sqrtf( 3.0f ) * 0.25f * triangleSize} }, /*R*/{ {1.0f, 0.0f, 0.0f} }  },
		{ /* t*/ { {                0.0f, -sqrtf( 3.0f ) * 0.25f * triangleSize} }, /*G*/{ {0.0f, 1.0f, 0.0f} }  },
		{ /*lb*/ { {-0.5f * triangleSize,  sqrtf( 3.0f ) * 0.25f * triangleSize} }, /*B*/{ {0.0f, 0.0f, 1.0f} }  }
	};

	const auto supportedLayers = enumerate<VkInstance, VkLayerProperties>();
	vector<const char*> requestedLayers;

#if VULKAN_VALIDATION
	if(  isLayerSupported( "VK_LAYER_KHRONOS_validation", supportedLayers )  ) requestedLayers.push_back( "VK_LAYER_KHRONOS_validation" );
	else throw "VULKAN_VALIDATION is enabled but VK_LAYER_KHRONOS_validation layers are not supported!";

	if( VulkanConfig::useAssistantLayer ){
		if(  isLayerSupported( "VK_LAYER_LUNARG_assistant_layer", supportedLayers )  ) requestedLayers.push_back( "VK_LAYER_LUNARG_assistant_layer" );
		else throw "VULKAN_VALIDATION is enabled but VK_LAYER_LUNARG_assistant_layer layer is not supported!";
	}
#endif

	if( VulkanConfig::fpsCounter ) requestedLayers.push_back( "VK_LAYER_LUNARG_monitor" );
	requestedLayers = checkInstanceLayerSupport( requestedLayers, supportedLayers );


	const auto supportedInstanceExtensions = getSupportedInstanceExtensions( requestedLayers );
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


	const VkInstance instance = initInstance( requestedLayers, requestedInstanceExtensions );

#if VULKAN_VALIDATION
	const auto debugHandle = initDebug( instance, debugExtensionTag, VulkanConfig::debugSeverity, VulkanConfig::debugType );

	const int32_t uncoded = 0;
	const char* introMsg = "Validation Layers are enabled!";
	if( debugExtensionTag == DebugObjectType::debugUtils ){
		VkDebugUtilsObjectNameInfoEXT object = {
			VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			nullptr, // pNext
			VK_OBJECT_TYPE_INSTANCE,
			handleToUint64(instance),
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
		vkSubmitDebugUtilsMessageEXT( instance, VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, &dumcd );
	}
	else if( debugExtensionTag == DebugObjectType::debugReport ){
		vkDebugReportMessageEXT( instance, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, (uint64_t)instance, __LINE__, uncoded, "Application", introMsg );
	}
#endif


	const PlatformWindow window = initWindow( VulkanConfig::appName, VulkanConfig::initialWindowWidth, VulkanConfig::initialWindowHeight );
	const VkSurfaceKHR surface = initSurface( instance, window );

	const VkPhysicalDevice physicalDevice = getPhysicalDevice( instance, surface );
	const VkPhysicalDeviceProperties physicalDeviceProperties = getPhysicalDeviceProperties( physicalDevice );
	const VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties = getPhysicalDeviceMemoryProperties( physicalDevice );

	uint32_t graphicsQueueFamily, presentQueueFamily;
	std::tie( graphicsQueueFamily, presentQueueFamily ) = getQueueFamilies( physicalDevice, surface );

	const VkPhysicalDeviceFeatures features = {}; // don't need any special feature for this demo
#ifdef __APPLE__ //
	const vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset" };
#else
	const vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
#endif

	const VkDevice device = initDevice( physicalDevice, features, graphicsQueueFamily, presentQueueFamily, requestedLayers, deviceExtensions );
	const VkQueue graphicsQueue = getQueue( device, graphicsQueueFamily, 0 );
	const VkQueue presentQueue = getQueue( device, presentQueueFamily, 0 );


	VkSurfaceFormatKHR surfaceFormat = getSurfaceFormat( physicalDevice, surface );
	VkRenderPass renderPass = initRenderPass( device, surfaceFormat );

	vector<uint32_t> vertexShaderBinary = {
#include "shaders/hello_triangle.vert.spv.inl"
	};
	vector<uint32_t> fragmentShaderBinary = {
#include "shaders/hello_triangle.frag.spv.inl"
	};
	VkShaderModule vertexShader = initShaderModule( device, vertexShaderBinary );
	VkShaderModule fragmentShader = initShaderModule( device, fragmentShaderBinary );
	VkPipelineLayout pipelineLayout = initPipelineLayout( device );

	VkBuffer vertexBuffer = initBuffer( device, sizeof( decltype( triangle )::value_type ) * triangle.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
	const std::vector<VkMemoryPropertyFlags> memoryTypePriority{
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // preferably wanna device-side memory that can be updated from host without hassle
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT // guaranteed to allways be supported
	};
	VkDeviceMemory vertexBufferMemory = initMemory<ResourceType::Buffer>(
		device,
		physicalDeviceMemoryProperties,
		vertexBuffer,
		memoryTypePriority
	);
	setVertexData( device, vertexBufferMemory, triangle ); // Writes throug memory map. Synchronization is implicit for any subsequent vkQueueSubmit batches.

	VkCommandPool commandPool = initCommandPool( device, graphicsQueueFamily );

	// might need synchronization if init is more advanced than this
	//VkResult errorCode = vkDeviceWaitIdle( device ); RESULT_HANDLER( errorCode, "vkDeviceWaitIdle" );


	// place-holder swapchain dependent objects
	VkSwapchainKHR swapchain = VK_NULL_HANDLE; // has to be NULL -- signifies that there's no swapchain
	vector<VkImageView> swapchainImageViews;
	vector<VkFramebuffer> framebuffers;

	VkPipeline pipeline = VK_NULL_HANDLE; // has to be NULL for the case the app ends before even first swapchain
	vector<VkCommandBuffer> commandBuffers;

	vector<VkSemaphore> imageReadySs;
	vector<VkSemaphore> renderDoneSs;

	// workaround for validation layer "memory leak" + might also help the driver to cleanup old resources
	// this should not be needed for a real-word app, because they are likely to use fences naturaly (e.g. responding to user input )
	// read https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues/1628
	const uint32_t maxInflightSubmissions = 2; // more than 2 probably does not make much sense
	uint32_t submissionNr = 0; // index of the current submission modulo maxInflightSubmission
	vector<VkFence> submissionFences;


	const std::function<bool(void)> recreateSwapchain = [&](){
		// swapchain recreation -- will be done before the first frame too;
		TODO( "This may be triggered from many sources (e.g. WM_SIZE event, and VK_ERROR_OUT_OF_DATE_KHR too). Should prevent duplicate swapchain recreation." )

		const VkSwapchainKHR oldSwapchain = swapchain;
		swapchain = VK_NULL_HANDLE;

		VkSurfaceCapabilitiesKHR capabilities = getSurfaceCapabilities( physicalDevice, surface );

		if( capabilities.currentExtent.width == UINT32_MAX && capabilities.currentExtent.height == UINT32_MAX ){
			capabilities.currentExtent.width = getWindowWidth( window );
			capabilities.currentExtent.height = getWindowHeight( window );
		}
		VkExtent2D surfaceSize = { capabilities.currentExtent.width, capabilities.currentExtent.height };

		const bool swapchainCreatable = {
			   surfaceSize.width >= capabilities.minImageExtent.width
			&& surfaceSize.width <= capabilities.maxImageExtent.width
			&& surfaceSize.width > 0
			&& surfaceSize.height >= capabilities.minImageExtent.height
			&& surfaceSize.height <= capabilities.maxImageExtent.height
			&& surfaceSize.height > 0
		};


		// cleanup old
		vector<VkSemaphore> oldImageReadySs = imageReadySs; imageReadySs.clear();
		if( oldSwapchain ){
			{VkResult errorCode = vkDeviceWaitIdle( device ); RESULT_HANDLER( errorCode, "vkDeviceWaitIdle" );}

			// fences might be in unsignaled state, so kill them too to get fresh signaled
			killFences( device, submissionFences );

			// semaphores might be in signaled state, so kill them too to get fresh unsignaled
			killSemaphores( device, renderDoneSs );
			// kill imageReadySs later when oldSwapchain is destroyed

			// only reset + later reuse already allocated and create new only if needed
			{VkResult errorCode = vkResetCommandPool( device, commandPool, 0 ); RESULT_HANDLER( errorCode, "vkResetCommandPool" );}

			killPipeline( device, pipeline );
			killFramebuffers( device, framebuffers );
			killSwapchainImageViews( device, swapchainImageViews );

			// kill oldSwapchain later, after it is potentially used by vkCreateSwapchainKHR
		}

		// creating new
		if( swapchainCreatable ){
			// reuses & destroys the oldSwapchain
			swapchain = initSwapchain( physicalDevice, device, surface, surfaceFormat, capabilities, graphicsQueueFamily, presentQueueFamily, oldSwapchain );

			vector<VkImage> swapchainImages = enumerate<VkImage>( device, swapchain );
			swapchainImageViews = initSwapchainImageViews( device, swapchainImages, surfaceFormat.format );
			framebuffers = initFramebuffers( device, renderPass, swapchainImageViews, surfaceSize.width, surfaceSize.height );

			pipeline = initPipeline(
				device,
				physicalDeviceProperties.limits,
				pipelineLayout,
				renderPass,
				vertexShader,
				fragmentShader,
				vertexBufferBinding,
				surfaceSize.width, surfaceSize.height
			);

			acquireCommandBuffers(  device, commandPool, static_cast<uint32_t>( swapchainImages.size() ), commandBuffers  );
			for( size_t i = 0; i < swapchainImages.size(); ++i ){
				beginCommandBuffer( commandBuffers[i] );
					recordBeginRenderPass( commandBuffers[i], renderPass, framebuffers[i], VulkanConfig::clearColor, surfaceSize.width, surfaceSize.height );

					recordBindPipeline( commandBuffers[i], pipeline );
					recordBindVertexBuffer( commandBuffers[i], vertexBufferBinding, vertexBuffer );

					recordDraw(  commandBuffers[i], static_cast<uint32_t>( triangle.size() )  );

					recordEndRenderPass( commandBuffers[i] );
				endCommandBuffer( commandBuffers[i] );
			}

			imageReadySs = initSemaphores( device, maxInflightSubmissions );
			// per https://github.com/KhronosGroup/Vulkan-Docs/issues/1150 need upto swapchain-image count
			renderDoneSs = initSemaphores( device, swapchainImages.size());

			submissionFences = initFences( device, maxInflightSubmissions, VK_FENCE_CREATE_SIGNALED_BIT ); // signaled fence means previous execution finished, so we start rendering presignaled
			submissionNr = 0;
		}

		if( oldSwapchain ){
			killSwapchain( device, oldSwapchain );

			// per current spec, we can't really be sure these are not used :/ at least kill them after the swapchain
			// https://github.com/KhronosGroup/Vulkan-Docs/issues/152
			killSemaphores( device, oldImageReadySs );
		}

		return swapchain != VK_NULL_HANDLE;
	};


	// Finally, rendering! Yay!
	const std::function<void(void)> render = [&](){
		assert( swapchain ); // should be always true; should have yielded CPU if false

		// vkAcquireNextImageKHR produces unsafe semaphore that needs extra cleanup. Track that with this variable.
		bool unsafeSemaphore = false;

		try{
			// remove oldest frame from being in flight before starting new one
			// refer to doc/, which talks about the cycle of how the synch primitives are (re)used here
			{VkResult errorCode = vkWaitForFences( device, 1, &submissionFences[submissionNr], VK_TRUE, UINT64_MAX ); RESULT_HANDLER( errorCode, "vkWaitForFences" );}
			{VkResult errorCode = vkResetFences( device, 1, &submissionFences[submissionNr] ); RESULT_HANDLER( errorCode, "vkResetFences" );}

			unsafeSemaphore = true;
			uint32_t nextSwapchainImageIndex = getNextImageIndex( device, swapchain, imageReadySs[submissionNr] );
			unsafeSemaphore = false;

			submitToQueue( graphicsQueue, commandBuffers[nextSwapchainImageIndex], imageReadySs[submissionNr], renderDoneSs[nextSwapchainImageIndex], submissionFences[submissionNr] );
			present( presentQueue, swapchain, nextSwapchainImageIndex, renderDoneSs[nextSwapchainImageIndex] );

			submissionNr = (submissionNr + 1) % maxInflightSubmissions;
		}
		catch( VulkanResultException ex ){
			if( ex.result == VK_SUBOPTIMAL_KHR || ex.result == VK_ERROR_OUT_OF_DATE_KHR ){
				if( unsafeSemaphore && ex.result == VK_SUBOPTIMAL_KHR ){
					cleanupUnsafeSemaphore( graphicsQueue, imageReadySs[submissionNr] );
					// no way to sanitize vkQueuePresentKHR semaphores, really
				}
				recreateSwapchain();

				// we need to start over...
				render();
			}
			else throw;
		}
	};


	setSizeEventHandler( recreateSwapchain );
	setPaintEventHandler( render );


	// Finally start the main message loop (and so render too)
	showWindow( window );
	int exitStatus = messageLoop( window );


	// proper Vulkan cleanup
	VkResult errorCode = vkDeviceWaitIdle( device ); RESULT_HANDLER( errorCode, "vkDeviceWaitIdle" );


	// kill swapchain
	killSemaphores( device, renderDoneSs );
	// imageReadySs killed after the swapchain

	// command buffers killed with pool

	killPipeline( device, pipeline );

	killFramebuffers( device, framebuffers );

	killSwapchainImageViews( device, swapchainImageViews );
	killSwapchain( device, swapchain );

	// per current spec, we can't really be sure these are not used :/ at least kill them after the swapchain
	// https://github.com/KhronosGroup/Vulkan-Docs/issues/152
	killSemaphores( device, imageReadySs );


	// kill vulkan
	killFences( device, submissionFences );

	killCommandPool( device,  commandPool );

	killMemory( device, vertexBufferMemory );
	killBuffer( device, vertexBuffer );

	killPipelineLayout( device, pipelineLayout );
	killShaderModule( device, fragmentShader );
	killShaderModule( device, vertexShader );

	killRenderPass( device, renderPass );

	killDevice( device );

	killSurface( instance, surface );
	killWindow( window );

#if VULKAN_VALIDATION
	killDebug( instance, debugHandle );
#endif
	killInstance( instance );

	return exitStatus;
}
catch( VulkanResultException vkE ){
	logger << "ERROR: Terminated due to an uncaught VkResult exception: "
	       << vkE.file << ":" << vkE.line << ":" << vkE.func << "() " << vkE.source << "() returned " << to_string( vkE.result )
	       << std::endl;
	return EXIT_FAILURE;
}
catch( const char* e ){
	logger << "ERROR: Terminated due to an uncaught exception: " << e << std::endl;
	return EXIT_FAILURE;
}
catch( string e ){
	logger << "ERROR: Terminated due to an uncaught exception: " << e << std::endl;
	return EXIT_FAILURE;
}
catch( std::exception e ){
	logger << "ERROR: Terminated due to an uncaught exception: " << e.what() << std::endl;
	return EXIT_FAILURE;
}
catch( ... ){
	logger << "ERROR: Terminated due to an unrecognized uncaught exception." << std::endl;
	return EXIT_FAILURE;
}


#if defined(_WIN32) && !defined(_CONSOLE)
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int ){
	return helloTriangle();
}
#else
int main(){
	return helloTriangle();
}
#endif

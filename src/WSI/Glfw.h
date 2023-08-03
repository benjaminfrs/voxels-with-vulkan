// GLFW platform dependent WSI handling and event loop

#ifndef COMMON_GLFW_WSI_H
#define COMMON_GLFW_WSI_H

#include <functional>
#include <string>
#include <queue>

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_NONE // Actually means include no OpenGL header
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "CompilerMessages.h"
#include "ErrorHandling.h"


TODO( "Easier to use, but might prevent platform co-existence. Could be namespaced. Make all of this a class?" )
struct PlatformWindow{ GLFWwindow* window; };

void setSizeEventHandler( std::function<bool(void)> newSizeEventHandler );
void setPaintEventHandler( std::function<void(void)> newPaintEventHandler );

struct GlfwError{
	int error;
	std::string description;
};


void glfwErrorCallback( int error, const char* description );


// just make the global glfw instialization\destruction static
class GlfwSingleton{
	static const GlfwSingleton glfwInstance;

	void destroyGlfw() noexcept{
		glfwTerminate();
		glfwSetErrorCallback( nullptr );
	}

	~GlfwSingleton(){
		this->destroyGlfw();
	}

	GlfwSingleton(){
		glfwSetErrorCallback( glfwErrorCallback );

		const auto success = glfwInit();
		if( !success ){
			glfwSetErrorCallback( nullptr );
			throw "Trouble initializing GLFW!";
		}

		if( !glfwVulkanSupported() ){
			this->destroyGlfw();
			throw "GLFW has trouble acquiring Vulkan support!";
		}
	}
};

void showWindow( PlatformWindow window );
std::string getPlatformSurfaceExtensionName();
GLFWmonitor* getCurrentMonitor( GLFWwindow* window );

void windowSizeCallback( GLFWwindow*, int, int ) noexcept;
void windowRefreshCallback( GLFWwindow* ) noexcept;
void toggleFullscreen( GLFWwindow* window );

TODO( "Fullscreen window seem to become unresponsive in Wayland session Ubuntu" )
void keyCallback( GLFWwindow* window, int key, int /*scancode*/, int action, int mods ) noexcept;
int messageLoop( PlatformWindow window );





bool platformPresentationSupport( VkInstance instance, VkPhysicalDevice device, uint32_t queueFamilyIndex, PlatformWindow );
PlatformWindow initWindow( const std::string& name, const uint32_t canvasWidth, const uint32_t canvasHeight );
VkSurfaceKHR initSurface( const VkInstance instance, const PlatformWindow window );

void killWindow( PlatformWindow window );

#endif //COMMON_GLFW_WSI_H

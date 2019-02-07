#include "src/startup.h"
#include "src/window.h"
#include "src/surface.h"
#include "src/swapchain.h"
#include "src/control.h"

#define number_of_queues 2 // <- change this if more queues needed

/* {
GVAR: window_width -> window.cpp
GVAR: window_height -> window.cpp
GVAR: swapchain -> window.cpp
GVAR: old_swapchain -> window.cpp
GVAR: AcquiredSemaphore -> window.cpp
GVAR: ReadySemaphore -> window.cpp
GVAR: compute_queue_family_index -> window.cpp
GVAR: graphics_queue_family_index -> window.cpp
GVAR: instance -> startup.cpp
} */

int main(int argc, char *lpCmdLine[])
{
	static const char *extensions[] =
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME

#elif defined VK_USE_PLATFORM_XCB_KHR
		VK_KHR_XCB_SURFACE_EXTENSION_NAME

#elif defined VK_USE_PLATFORM_XLIB_KHR
		VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#endif
	};
	static const char *device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	struct QueueInfo QueueInfos[number_of_queues];
	WindowParameters windowParams;
	float priority[] = {1.0f};
	static VkImageUsageFlags desired_usages = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |  VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	static VkSurfaceTransformFlagBitsKHR desired_transform;

	initWindow();

	if (
	    !LoadVulkan() ||
	    !LoadVulkanGlobalFuncs() ||
	    !CheckInstanceExtensions() ||
	    !CreateVulkanInstance(2,extensions)||
	    !LoadInstanceFunctions() ||
	    !CheckPhysicalDevices() ||
	    !CheckPhysicalDeviceExtensions()||
	    !CheckQueueProperties(VK_QUEUE_GRAPHICS_BIT, graphics_queue_family_index )||
	    !CheckQueueProperties(VK_QUEUE_COMPUTE_BIT, compute_queue_family_index)||
	    //! SURFACE INIT
	    !CreatePresentationSurface(windowParams)||
	    !CheckSurfaceQueueSupport(graphics_queue_family_index)||
	    !CheckSurfaceQueueSupport(compute_queue_family_index)||
	    !CheckSelectPresentationModesSupport(VK_PRESENT_MODE_MAILBOX_KHR)||
	    !CheckPresentationSurfaceCapabilities()||
	    //~ SURFACE INIT
	    !SetQueue(QueueInfos, graphics_queue_family_index, priority, 0)||
	    !SetQueue(QueueInfos, compute_queue_family_index, priority, 1)||
	    !CreateLogicalDevice(QueueInfos, number_of_queues, 1, device_extensions)||
	    !LoadDeviceLevelFunctions())
	{
		debug_pause();
		exit(1);
	}

	vkGetDeviceQueue(logical_device, graphics_queue_family_index, 0, &GraphicsQueue);
	vkGetDeviceQueue(logical_device, compute_queue_family_index, 0, &ComputeQueue);

	std::cout << "Vulkan Initialized Successfully!" << std::endl;

#ifdef DEBUG
	VkDebugReportCallbackEXT debugCallback = registerDebugCallback();
#endif

	if(
	    !SelectNumberOfSwapchainImages()||
	    !ComputeSizeOfSwapchainImages(window_width, window_height)||
	    !SelectDesiredUsageScenariosOfSwapchainImages(desired_usages)||
	    !SelectTransformationOfSwapchainImages(desired_transform)||
	    !SelectFormatOfSwapchainImages({VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})||
	    !CreateSwapchain(swapchain, old_swapchain)||
	    !GetHandlesOfSwapchainImages(swapchain)
	)
	{
		debug_pause();
		exit(1);
	}

	std::cout << "Swapchain Created!" << std::endl;

	if(
	    !CreateSemaphore(AcquiredSemaphore)||
	    !CreateSemaphore(ReadySemaphore)||
	    !CreateCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, graphics_queue_family_index)||
	    !AllocateCommandBuffers(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1)
	)
	{
		debug_pause();
		exit(1);
	}

	std::cout << "Initial Control Buffer Created!" << std::endl;

	mainLoop();

#ifdef DEBUG
	if(debugCallback != 0)
	{
		vkDestroyDebugReportCallbackEXT(instance, debugCallback, 0);
	}
#endif

	ReleaseVulkanLoaderLibrary();

	debug_pause();
}

#include "src/startup.h"
#include "src/window.h"
#include "src/surface.h"

#define number_of_queues 2 // <- change this if more queues needed

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
	uint32_t graphics_queue_family_index; // <- this is queue 1
	uint32_t compute_queue_family_index; // <- this is queue 2
	VkQueue GraphicsQueue;
	VkQueue ComputeQueue;
	struct QueueInfo QueueInfos[number_of_queues];
	WindowParameters windowParams;
	float priority[] = {1.0f};
	VkImageUsageFlags desired_usages = 1;
	VkSurfaceTransformFlagBitsKHR desired_transform;
	
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
		!CreatePresentationSurface(windowParams)||
		!CheckSurfaceQueueSupport(graphics_queue_family_index)||
		!CheckSurfaceQueueSupport(compute_queue_family_index)||
		!CheckSelectPresentationModesSupport(VK_PRESENT_MODE_MAILBOX_KHR)||
		!CheckPresentationSurfaceCapabilities()||
		!SelectNumberOfSwapchainImages()||
		!ComputeSizeOfSwapchainImages(640, 480)||
		!SelectDesiredUsageScenariosOfSwapchainImages(desired_usages)|| 
		!SelectTransformationOfSwapchainImages(desired_transform)||
		!SelectFormatOfSwapchainImages({VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})||
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
	
	mainLoop();
	
	ReleaseVulkanLoaderLibrary();
	
	debug_pause();
}

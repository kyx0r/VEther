#include "src/startup.h"
#include "src/window.h"
#include "src/surface.h"
#include "src/swapchain.h"
#include "src/control.h"
#include "src/shaders.h"
#include "src/textures.h"
#include "src/flog.h"

#define number_of_queues 1 // <- change this if more queues needed

/* {
GVAR: window_width -> window.cpp
GVAR: window_height -> window.cpp
GVAR: _swapchain -> window.cpp
GVAR: old_swapchain -> window.cpp
GVAR: AcquiredSemaphore -> window.cpp
GVAR: ReadySemaphore -> window.cpp
GVAR: compute_queue_family_index -> window.cpp
GVAR: graphics_queue_family_index -> window.cpp
GVAR: instance -> startup.cpp
GVAR: Fence_one -> window.cpp
GVAR: memory_properties -> control.cpp
GVAR: target_device -> startup.cpp
GVAR: NUM_COMMAND_BUFFERS -> control.cpp
} */

int main(int argc, char *lpCmdLine[])
{
	static const char *extensions[] =
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,

#elif defined VK_USE_PLATFORM_XCB_KHR
		VK_KHR_XCB_SURFACE_EXTENSION_NAME,

#elif defined VK_USE_PLATFORM_XLIB_KHR
		VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#endif

#ifdef DEBUG
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
	};

	static const char *device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	struct QueueInfo QueueInfos[number_of_queues];
	WindowParameters windowParams;
	float priority[] = {0.0f};
	static VkImageUsageFlags desired_usages = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |  VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	static VkSurfaceTransformFlagBitsKHR desired_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

	zone::Memory_Init(malloc(DEFAULT_MEMORY), DEFAULT_MEMORY);
	
	log_set_level(0);
	FILE* f = fopen("./log.txt","w");
	log_set_fp(f);
	
	window::initWindow();

	if (
	    !startup::LoadVulkan() ||
	    !startup::LoadVulkanGlobalFuncs() ||
	    !startup::CheckInstanceExtensions() ||
	    !startup::CreateVulkanInstance(ARRAYSIZE(extensions),extensions)||
	    !startup::LoadInstanceFunctions())
	{
		startup::debug_pause();
		exit(1);
	}

#ifdef DEBUG
	VkDebugReportCallbackEXT debugCallback = startup::registerDebugCallback();
#endif

	if(
	    !startup::CheckPhysicalDevices() ||
	    !startup::CheckPhysicalDeviceExtensions()||
	    !startup::CheckQueueProperties(VK_QUEUE_GRAPHICS_BIT, graphics_queue_family_index )||
	    //  !startup::CheckQueueProperties(VK_QUEUE_COMPUTE_BIT, compute_queue_family_index)||
	    !surface::CreatePresentationSurface(windowParams)||
	    !surface::CheckSurfaceQueueSupport(graphics_queue_family_index)||
	    //!surface::CheckSurfaceQueueSupport(compute_queue_family_index)||
	    !surface::CheckSelectPresentationModesSupport(VK_PRESENT_MODE_MAILBOX_KHR)||
	    !surface::CheckPresentationSurfaceCapabilities()||
	    !startup::SetQueue(QueueInfos, graphics_queue_family_index, priority, 0)||
	    // !startup::SetQueue(QueueInfos, compute_queue_family_index, priority, 1)||
	    !startup::CreateLogicalDevice(QueueInfos, number_of_queues, 1, device_extensions)||
	    !startup::LoadDeviceLevelFunctions())
	{
		startup::debug_pause();
		exit(1);
	}

	vkGetDeviceQueue(logical_device, graphics_queue_family_index, 0, &GraphicsQueue);
	//vkGetDeviceQueue(logical_device, compute_queue_family_index, 0, &ComputeQueue);

	trace("Vulkan Initialized Successfully! \n");

	if(
	    !swapchain::SelectNumberOfSwapchainImages()||
	    !swapchain::ComputeSizeOfSwapchainImages(window_width, window_height)||
	    !swapchain::SelectDesiredUsageScenariosOfSwapchainImages(desired_usages)||
	    !swapchain::SelectTransformationOfSwapchainImages(desired_transform)||
	    !swapchain::SelectFormatOfSwapchainImages({VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})||
	    !swapchain::CreateSwapchain(_swapchain, old_swapchain)||
	    !swapchain::GetHandlesOfSwapchainImages(_swapchain)
	)
	{
		startup::debug_pause();
		exit(1);
	}

	trace("Swapchain Created! \n");

	if(
	    !control::CreateSemaphore(AcquiredSemaphore)||
	    !control::CreateSemaphore(ReadySemaphore)||
	    !control::CreateFence(Fence_one, VK_FENCE_CREATE_SIGNALED_BIT)||
	    !control::CreateCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, graphics_queue_family_index)||
	    !control::AllocateCommandBuffers(VK_COMMAND_BUFFER_LEVEL_PRIMARY, NUM_COMMAND_BUFFERS)
	)
	{
		startup::debug_pause();
		exit(1);
	}

	trace("Initial Control Buffer Created! \n");

	vkGetPhysicalDeviceMemoryProperties(target_device, &memory_properties);
	control::IndexBuffersAllocate();
	control::VertexBuffersAllocate();
	control::StagingBuffersAllocate();
	control::UniformBuffersAllocate();

	textures::InitSamplers();
	textures::GenerateColorPalette();

	window::mainLoop();

#ifdef DEBUG
	if(debugCallback != 0)
	{
		vkDestroyDebugReportCallbackEXT(instance, debugCallback, 0);
	}
#endif

	startup::ReleaseVulkanLoaderLibrary();
	//	startup::debug_pause();
}

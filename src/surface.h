#include "window.h"

struct WindowParameters
{
	#ifdef VK_USE_PLATFORM_WIN32_KHR
	HINSTANCE HInstance;
	HWND HWnd;
	#elif defined VK_USE_PLATFORM_XLIB_KHR
	Display *Dpy;
	Window Window;
	#elif defined VK_USE_PLATFORM_XCB_KHR
	xcb_connection_t *Connection;
	xcb_window_t *Window;
	#endif
};

extern VkSurfaceKHR presentation_surface;

//------------------------------

bool CreatePresentationSurface(WindowParameters &window_parameters);
bool CheckSurfaceQueueSupport(uint32_t &queue_family_index);
bool CheckSelectPresentationModesSupport(VkPresentModeKHR desired_mode);
bool CheckPresentationSurfaceCapabilities();
bool SelectNumberOfSwapchainImages();
bool ComputeSizeOfSwapchainImages(uint32_t x, uint32_t y);
bool SelectDesiredUsageScenariosOfSwapchainImages(VkImageUsageFlags desired_usages); 
//how the image is transformed when rendering. 
bool SelectTransformationOfSwapchainImages(VkSurfaceTransformFlagBitsKHR desired_transform);
//set surface format and its matching color format 
bool SelectFormatOfSwapchainImages(VkSurfaceFormatKHR desired_surface_format);

//------------------------------
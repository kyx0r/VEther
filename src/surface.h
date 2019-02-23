#include "window.h"

extern VkSurfaceKHR presentation_surface;
extern VkPresentModeKHR target_surface_mode;
extern VkSurfaceCapabilitiesKHR surface_capabilities;

#pragma once
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

//------------------------------

namespace surface
{

bool CreatePresentationSurface(WindowParameters &window_parameters);
void DestroyPresentationSurface();
bool CheckSurfaceQueueSupport(uint32_t &queue_family_index);
bool CheckSelectPresentationModesSupport(VkPresentModeKHR desired_mode);
bool CheckPresentationSurfaceCapabilities();

}
//------------------------------
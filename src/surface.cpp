#include "surface.h"
#include "flog.h"
/* {
GVAR: target_device -> startup.cpp
GVAR: instance -> startup.cpp
GVAR: _window ->window.cpp
} */

//{
VkSurfaceKHR presentation_surface;
VkPresentModeKHR target_surface_mode;
VkSurfaceCapabilitiesKHR surface_capabilities;
//}

namespace surface
{

bool CreatePresentationSurface(WindowParameters &window_parameters)
{
	VkResult result;

#ifdef VK_USE_PLATFORM_WIN32_KHR
	window_parameters.HWnd = glfwGetWin32Window(_window);
	window_parameters.HInstance = GetModuleHandle(nullptr);
	VkWin32SurfaceCreateInfoKHR surface_create_info =
	{
		VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,  // VkStructureType                 sType
		nullptr,                                          // const void                    * pNext
		0,                                                // VkWin32SurfaceCreateFlagsKHR    flags
		window_parameters.HInstance,                      // HINSTANCE                       hinstance
		window_parameters.HWnd                            // HWND                            hwnd
	};

	result = vkCreateWin32SurfaceKHR( instance, &surface_create_info, nullptr, &presentation_surface );

#elif defined VK_USE_PLATFORM_XLIB_KHR

	window_parameters.wWindow = glfwGetX11Window(_window);
	window_parameters.Dpy = glfwGetX11Display();

	VkXlibSurfaceCreateInfoKHR surface_create_info =
	{
		VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,   // VkStructureType                 sType
		nullptr,                                          // const void                    * pNext
		0,                                                // VkXlibSurfaceCreateFlagsKHR     flags
		window_parameters.Dpy,                            // Display                       * dpy
		window_parameters.wWindow                          // Window                          window
	};

	result = vkCreateXlibSurfaceKHR( instance, &surface_create_info, nullptr, &presentation_surface );

#elif defined VK_USE_PLATFORM_XCB_KHR

	window_parameters.wWindow = glfwGetX11Window(_window);
	window_parameters.Connection = glfwGetConnection();

	VkXcbSurfaceCreateInfoKHR surface_create_info =
	{
		VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,    // VkStructureType                 sType
		nullptr,                                          // const void                    * pNext
		0,                                                // VkXcbSurfaceCreateFlagsKHR      flags
		window_parameters.Connection,                     // xcb_connection_t              * connection
		window_parameters.wWindow                          // xcb_window_t                    window
	};
	result = vkCreateXcbSurfaceKHR( instance, &surface_create_info, nullptr, &presentation_surface );

#endif

	if( (VK_SUCCESS != result) ||
	        (VK_NULL_HANDLE == presentation_surface) )
	{
		fatal("Could not create presentation surface.");
		return false;
	}
	return true;
}

bool CheckSurfaceQueueSupport(uint32_t &queue_family_index)
{
	for(uint32_t i = 0; i<queue_families_count; ++i)
	{
		VkBool32 presentation_supported = VK_FALSE;
		VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(target_device, i, presentation_surface, &presentation_supported);
		if(result == VK_SUCCESS && presentation_supported == VK_TRUE)
		{
			queue_family_index = i;
			return true;
		}
	}
	fatal("Surface queue is not supported by the device.");
	return false;
}

bool CheckSelectPresentationModesSupport(VkPresentModeKHR desired_mode)
{
	uint32_t modes_count = 0;
	VkResult result = VK_SUCCESS;
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(target_device, presentation_surface, &modes_count, nullptr);
	if(result != VK_SUCCESS || modes_count == 0)
	{
		fatal("Could not get number of surface presentation modes!");
		return false;
	}
	VkPresentModeKHR present_modes[modes_count];
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(target_device, presentation_surface, &modes_count, &present_modes[0]);
	if(result != VK_SUCCESS || modes_count == 0)
	{
		fatal("Could not enumerate surface presentation modes!");
		return false;
	}
	for(uint32_t i = 0; i<modes_count; i++)
	{
		if(present_modes[i]==desired_mode)
		{
			target_surface_mode = desired_mode;
			return true;
		}
	}
	info("Note: Desired mode is not supported. Selecting default FIFO");
	for(uint32_t i = 0; i<modes_count; i++)
	{
		if(present_modes[i]==VK_PRESENT_MODE_FIFO_KHR)
		{
			target_surface_mode = VK_PRESENT_MODE_FIFO_KHR;
			return true;
		}
	}
	error("Function failed - %s", __func__);
	return false;
}

bool CheckPresentationSurfaceCapabilities()
{
	VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(target_device, presentation_surface, &surface_capabilities);
	if(result != VK_SUCCESS)
	{
		fatal("Could not get capabilities of the presentation surface! %s",startup::GetVulkanResultString(result));
		return false;
	}
	return true;
}

void DestroyPresentationSurface()
{
	vkDestroySurfaceKHR(instance, presentation_surface, nullptr);
	presentation_surface = VK_NULL_HANDLE;
}

} //namespace surface

#include "surface.h"

VkSurfaceKHR presentation_surface; // <- exported to other files.
VkPresentModeKHR target_surface_mode; // <- exported to other files.
VkSurfaceCapabilitiesKHR surface_capabilities; // <- exported to other files.

//VkPresentModeKHR *present_modes = nullptr;

bool CreatePresentationSurface(WindowParameters &window_parameters)
{
    VkResult result;

#ifdef VK_USE_PLATFORM_WIN32_KHR
	window_parameters.HWnd = glfwGetWin32Window(window);
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

    VkXlibSurfaceCreateInfoKHR surface_create_info = 
	{
      VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,   // VkStructureType                 sType
      nullptr,                                          // const void                    * pNext
      0,                                                // VkXlibSurfaceCreateFlagsKHR     flags
      window_parameters.Dpy,                            // Display                       * dpy
      window_parameters.Window                          // Window                          window
    };

    result = vkCreateXlibSurfaceKHR( instance, &surface_create_info, nullptr, &presentation_surface );

#elif defined VK_USE_PLATFORM_XCB_KHR

    VkXcbSurfaceCreateInfoKHR surface_create_info = 
	{
      VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,    // VkStructureType                 sType
      nullptr,                                          // const void                    * pNext
      0,                                                // VkXcbSurfaceCreateFlagsKHR      flags
      window_parameters.Connection,                     // xcb_connection_t              * connection
      window_parameters.Window                          // xcb_window_t                    window
    };

    result = vkCreateXcbSurfaceKHR( instance, &surface_create_info, nullptr, &presentation_surface );

#endif

    if( (VK_SUCCESS != result) ||
        (VK_NULL_HANDLE == presentation_surface) ) {
      std::cout << "Could not create presentation surface." << std::endl;
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
	std::cout<<"Surface queue is not supported by the device. \n";
	return false;
}

bool CheckSelectPresentationModesSupport(VkPresentModeKHR desired_mode)
{
	uint32_t modes_count = 0;
	VkResult result = VK_SUCCESS;
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(target_device, presentation_surface, &modes_count, nullptr);
	if(result != VK_SUCCESS || modes_count == 0)
	{
		std::cout<<"Could not get number of surface presentation modes! \n";
		return false;
	}
	VkPresentModeKHR present_modes[modes_count];
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(target_device, presentation_surface, &modes_count, &present_modes[0]);
	if(result != VK_SUCCESS || modes_count == 0)
	{
		std::cout<<"Could not enumerate surface presentation modes! \n";
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
	std::cout<<"Note: Desired mode is not supported. Selecting default FIFO \n";
	for(uint32_t i = 0; i<modes_count; i++)
	{
		if(present_modes[i]==VK_PRESENT_MODE_FIFO_KHR)
		{
			target_surface_mode = VK_PRESENT_MODE_FIFO_KHR;
			return true;
		}
	}	
	std::cout<<"Function "<<__func__<<" failed!"<<std::endl;
	return false;
}

bool CheckPresentationSurfaceCapabilities()
{
	VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(target_device, presentation_surface, &surface_capabilities);
	if(result != VK_SUCCESS)
	{
		std::cout<<"Could not get capabilities of the presentation surface! "<<GetVulkanResultString(result)<<std::endl;
		return false;
	}
	return true;
}
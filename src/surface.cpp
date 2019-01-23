#include "surface.h"

VkSurfaceKHR presentation_surface;
VkPresentModeKHR *present_modes;
VkPresentModeKHR target_surface_mode;
VkSurfaceCapabilitiesKHR surface_capabilities;

uint32_t number_of_images;
VkExtent2D size_of_images;
VkImageUsageFlags image_usage;
VkFormat image_format;
VkColorSpaceKHR image_color_space;
VkSurfaceTransformFlagBitsKHR  surface_transform;

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

bool SelectNumberOfSwapchainImages()
{
    number_of_images = surface_capabilities.minImageCount + 1;
    if(surface_capabilities.maxImageCount > 0 && number_of_images > surface_capabilities.maxImageCount) 
	{
		number_of_images = surface_capabilities.maxImageCount;
    }
    return true;	
}

bool ComputeSizeOfSwapchainImages(uint32_t x, uint32_t y) 
{
    if( 0xFFFFFFFF == surface_capabilities.currentExtent.width ) 
	{
      size_of_images = { x, y };

      if( size_of_images.width < surface_capabilities.minImageExtent.width ) 
	  {
        size_of_images.width = surface_capabilities.minImageExtent.width;
      } else if( size_of_images.width > surface_capabilities.maxImageExtent.width ) 
	  {
        size_of_images.width = surface_capabilities.maxImageExtent.width;
      }

      if( size_of_images.height < surface_capabilities.minImageExtent.height ) 
	  {
        size_of_images.height = surface_capabilities.minImageExtent.height;
      } else if( size_of_images.height > surface_capabilities.maxImageExtent.height ) 
	  {
        size_of_images.height = surface_capabilities.maxImageExtent.height;
      }
    } else 
	{
      size_of_images = surface_capabilities.currentExtent;
    }
    return true;
}

bool SelectDesiredUsageScenariosOfSwapchainImages(VkImageUsageFlags desired_usages) 
{
    image_usage = desired_usages & surface_capabilities.supportedUsageFlags;
    return desired_usages == image_usage;
}

bool SelectTransformationOfSwapchainImages(VkSurfaceTransformFlagBitsKHR desired_transform)
{
    if( surface_capabilities.supportedTransforms & desired_transform ) {
      surface_transform = desired_transform;
    } else {
      surface_transform = surface_capabilities.currentTransform;
    }
    return true;
}

bool SelectFormatOfSwapchainImages(VkSurfaceFormatKHR desired_surface_format) 
{
    // Enumerate supported formats
    uint32_t formats_count = 0;
    VkResult result = VK_SUCCESS;

    result = vkGetPhysicalDeviceSurfaceFormatsKHR( target_device, presentation_surface, &formats_count, nullptr );
    if(result != VK_SUCCESS || formats_count == 0 ) 
	{
      std::cout << "Could not get the number of supported surface formats." << std::endl;
      return false;
    }

    VkSurfaceFormatKHR surface_formats [formats_count];
    result = vkGetPhysicalDeviceSurfaceFormatsKHR( target_device, presentation_surface, &formats_count, &surface_formats[0] );
    if(result != VK_SUCCESS || formats_count == 0)
	{
      std::cout << "Could not enumerate supported surface formats." << std::endl;
      return false;
    }

    // Select surface format
    if(formats_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) 
	{
      image_format = desired_surface_format.format;
      image_color_space = desired_surface_format.colorSpace;
      return true;
    }

    for(uint32_t i = 0; i<formats_count; i++) 
	{
      if(desired_surface_format.format == surface_formats[i].format && desired_surface_format.colorSpace == surface_formats[i].colorSpace)
	  {
        image_format = desired_surface_format.format;
        image_color_space = desired_surface_format.colorSpace;
        return true;
      }
    }

    for(uint32_t i = 0; i<formats_count; i++) 
	{
      if( (desired_surface_format.format == surface_formats[i].format) ) 
	  {
        image_format = desired_surface_format.format;
        image_color_space = surface_formats[i].colorSpace;
        std::cout << "Desired combination of format and colorspace is not supported. Selecting other colorspace." << std::endl;
        return true;
      }
    }
	
    image_format = surface_formats[0].format;
    image_color_space = surface_formats[0].colorSpace;
    std::cout << "Desired format is not supported. Selecting available format - colorspace combination." << std::endl;
    return true;
}
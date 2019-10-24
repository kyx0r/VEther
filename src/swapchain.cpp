#include "swapchain.h"
#include "zone.h"
/* {
GVAR: logical_device -> startup.cpp
GVAR: target_device -> startup.cpp
GVAR: surface_capabilities -> surface.cpp
GVAR: presentation_surface -> surface.cpp
} */

//{
VkImage *handle_array_of_swapchain_images = nullptr;
VkFormat image_format;
uint32_t number_of_swapchain_images;
//}

static VkExtent2D size_of_images;
static VkImageUsageFlags image_usage;
static VkColorSpaceKHR image_color_space;
static VkSurfaceTransformFlagBitsKHR  surface_transform;

namespace swapchain
{

bool SelectNumberOfSwapchainImages()
{
	number_of_swapchain_images = surface_capabilities.minImageCount + 1;
	if(surface_capabilities.maxImageCount > 0 && number_of_swapchain_images > surface_capabilities.maxImageCount)
	{
		number_of_swapchain_images = surface_capabilities.maxImageCount;		
	}
	//HACK: verify this behavior. 
	//keep one more because AcquireSwapchainImage may go out of bounds?
	number_of_swapchain_images += 1;	
	return true;
}

bool SetSizeOfSwapchainImages(uint32_t x, uint32_t y)
{
	size_of_images = { x, y };

	if( x < surface_capabilities.minImageExtent.width )
	{
		size_of_images.width = surface_capabilities.minImageExtent.width;
	}
	else if( x > surface_capabilities.maxImageExtent.width )
	{
		size_of_images.width = surface_capabilities.maxImageExtent.width;
	}

	if( y < surface_capabilities.minImageExtent.height )
	{
		size_of_images.height = surface_capabilities.minImageExtent.height;
	}
	else if( y > surface_capabilities.maxImageExtent.height )
	{
		size_of_images.height = surface_capabilities.maxImageExtent.height;
	}
	return true;
}

bool ComputeSizeOfSwapchainImages(uint32_t x, uint32_t y)
{
	if(surface_capabilities.currentExtent.width == 0xFFFFFFFF)
	{
		size_of_images = { x, y };

		if( size_of_images.width < surface_capabilities.minImageExtent.width )
		{
			size_of_images.width = surface_capabilities.minImageExtent.width;
		}
		else if( size_of_images.width > surface_capabilities.maxImageExtent.width )
		{
			size_of_images.width = surface_capabilities.maxImageExtent.width;
		}

		if( size_of_images.height < surface_capabilities.minImageExtent.height )
		{
			size_of_images.height = surface_capabilities.minImageExtent.height;
		}
		else if( size_of_images.height > surface_capabilities.maxImageExtent.height )
		{
			size_of_images.height = surface_capabilities.maxImageExtent.height;
		}
	}
	else
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
	if( surface_capabilities.supportedTransforms & desired_transform )
	{
		surface_transform = desired_transform;
	}
	else
	{
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

bool CreateSwapchain(VkSwapchainKHR &_swapchain, VkSwapchainKHR &old_swapchain)
{
	VkSwapchainCreateInfoKHR swapchain_create_info =
	{
		VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,  // VkStructureType                  sType
		nullptr,                                      // const void                     * pNext
		0,                                            // VkSwapchainCreateFlagsKHR        flags
		presentation_surface,                         // VkSurfaceKHR                     surface
		number_of_swapchain_images,                   // uint32_t                         minImageCount
		image_format,                                 // VkFormat                         imageFormat
		image_color_space,                            // VkColorSpaceKHR                  imageColorSpace
		size_of_images,                               // VkExtent2D                       imageExtent
		1,                                            // uint32_t                         imageArrayLayers
		image_usage,                                  // VkImageUsageFlags                imageUsage
		VK_SHARING_MODE_EXCLUSIVE,                    // VkSharingMode                    imageSharingMode
		0,                                            // uint32_t                         queueFamilyIndexCount
		nullptr,                                      // const uint32_t                 * pQueueFamilyIndices
		surface_transform,                            // VkSurfaceTransformFlagBitsKHR    preTransform
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,            // VkCompositeAlphaFlagBitsKHR      compositeAlpha
		target_surface_mode,                          // VkPresentModeKHR                 presentMode
		VK_TRUE,                                      // VkBool32                         clipped
		old_swapchain                                 // VkSwapchainKHR                   oldSwapchain
	};

	VkResult result = vkCreateSwapchainKHR( logical_device, &swapchain_create_info, nullptr, &_swapchain );
	if( (VK_SUCCESS != result) ||
	        (VK_NULL_HANDLE == _swapchain) )
	{
		std::cout << "Could not create a swapchain." << std::endl;
		return false;
	}

	if( VK_NULL_HANDLE != old_swapchain )
	{
		vkDestroySwapchainKHR( logical_device, old_swapchain, nullptr );
		old_swapchain = VK_NULL_HANDLE;
	}

	return true;
}

bool GetHandlesOfSwapchainImages(VkSwapchainKHR &_swapchain)
{
	uint32_t images_count = 0;
	VkResult result = VK_SUCCESS;

	result = vkGetSwapchainImagesKHR( logical_device, _swapchain, &images_count, nullptr );
	if(result != VK_SUCCESS || images_count == 0)
	{
		std::cout << "Could not get the number of swapchain images." << std::endl;
		return false;
	}

	if(handle_array_of_swapchain_images != nullptr)
	{
	  zone::Z_Free((char*)&handle_array_of_swapchain_images[0]);
	}
	char* mem = (char*) zone::Z_Malloc(sizeof(VkImage) * images_count);
	handle_array_of_swapchain_images = new(mem) VkImage [images_count];

	result = vkGetSwapchainImagesKHR( logical_device, _swapchain, &images_count, &handle_array_of_swapchain_images[0]);
	if(result != VK_SUCCESS || images_count == 0)
	{
		std::cout << "Could not enumerate swapchain images." << std::endl;
		return false;
	}
	return true;
}

bool AcquireSwapchainImage(VkSwapchainKHR _swapchain, VkSemaphore semaphore, VkFence fence, uint32_t &image_index)
{
	VkResult result;

	//tell hardware to not wait more than 2 seconds.
	result = vkAcquireNextImageKHR( logical_device, _swapchain, 2000000000, semaphore, fence, &image_index );
	switch( result )
	{
	case VK_SUCCESS:
	case VK_SUBOPTIMAL_KHR:
		return true;
	case VK_TIMEOUT:
	        printf("Note: VK_TIMEOUT\n");		
	        VK_CHECK(vkDeviceWaitIdle(logical_device));
	        return false;
	default:
		std::cout << startup::GetVulkanResultString(result) <<" in func "<<__func__<< std::endl;
		return false;
	}
}

void DestroySwapchain(VkSwapchainKHR &_swapchain)
{
	if(_swapchain)
	{
		vkDestroySwapchainKHR(logical_device, _swapchain, nullptr);
		_swapchain = VK_NULL_HANDLE;
	}
}

} // namespace swapchain

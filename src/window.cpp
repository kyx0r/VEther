#include "window.h"
#include "control.h"
#include "render.h"

/* {
GVAR: 
} */

//{
GLFWwindow* window;
VkSwapchainKHR swapchain = VK_NULL_HANDLE;
VkSwapchainKHR old_swapchain = VK_NULL_HANDLE;
VkSemaphore AcquiredSemaphore;
VkSemaphore ReadySemaphore;
VkQueue GraphicsQueue;
VkQueue ComputeQueue;
int window_width = 1280;
int window_height = 960;
uint32_t graphics_queue_family_index; // <- this is queue 1
uint32_t compute_queue_family_index; // <- this is queue 2
//}

void window_size_callback(GLFWwindow* window, int width, int height)
{
	WaitForAllSubmittedCommandsToBeFinished();
	old_swapchain = swapchain;
	swapchain = VK_NULL_HANDLE;
	if(
		!SetSizeOfSwapchainImages(width, height)||
		!CreateSwapchain(swapchain, old_swapchain)||
		!GetHandlesOfSwapchainImages(swapchain)
		)
	{
		debug_pause();
		exit(1);		
	}
	window_height = height;
	window_width = width;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_R)
		{
			
		}
		if (key == GLFW_KEY_C)
		{
			
		}
		if (key == GLFW_KEY_O)
		{
			
		}
		if (key == GLFW_KEY_L)
		{
			
		}
		if (key == GLFW_KEY_P)
		{
			
		}
	}
}

void initWindow() 
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(window_width, window_height, "VEther", nullptr, nullptr);
	glfwSetWindowSizeCallback(window, window_size_callback);
	glfwSetKeyCallback(window, keyCallback);
}

inline bool Draw()
{
	uint32_t image_index;
	VkResult result;
	WaitForAllSubmittedCommandsToBeFinished();
	if(
		!AcquireSwapchainImage(swapchain, AcquiredSemaphore, VK_NULL_HANDLE, image_index)||
		!BeginCommandBufferRecordingOperation(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr)
		)
	{
		return false;
	}
	
	VkClearColorValue color = 
	{
		1,
		0,
		1,
		1
	};
	
	VkImageSubresourceRange range = 
	{
		VK_IMAGE_ASPECT_COLOR_BIT,                // VkImageAspectFlags         aspectMask
        0,                                        // uint32_t                   baseMipLevel
        VK_REMAINING_MIP_LEVELS,                  // uint32_t                   levelCount
        0,                                        // uint32_t                   baseArrayLayer
        VK_REMAINING_ARRAY_LAYERS                 // uint32_t                   layerCount		
	};	
	
/* 	if(!CreateFramebuffer(VkImageView colorView, VkImageView depthView, window_width, window_height))
	{
		std::cout<<"Failed to create frame buffer ! \n";
		return false;
	} */
	
	//vkCmdClearColorImage(command_buffer, handle_array_of_swapchain_images[image_index], VK_IMAGE_LAYOUT_GENERAL, &color, 1, &range);
	
	VkImageMemoryBarrier image_memory_barrier_before_draw =
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		nullptr,                                  	// const void               * pNext
        0,           								// VkAccessFlags              srcAccessMask
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,       // VkAccessFlags              dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,           		// VkImageLayout              oldLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,   // VkImageLayout              newLayout
        VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                   srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                   dstQueueFamilyIndex
        handle_array_of_swapchain_images[image_index],   // VkImage                    image
		range
	};
	SetImageMemoryBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, image_memory_barrier_before_draw);
	
	VkImageMemoryBarrier image_memory_barrier_before_present =
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		nullptr,                                  	// const void               * pNext
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,       // VkAccessFlags              srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,       			// VkAccessFlags              dstAccessMask
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,           		// VkImageLayout              oldLayout
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,   			// VkImageLayout              newLayout
        VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                   srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                   dstQueueFamilyIndex
        handle_array_of_swapchain_images[image_index],   // VkImage                    image
        range
	};
	SetImageMemoryBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, image_memory_barrier_before_present);

	if(!EndCommandBufferRecordingOperation())
	{
		return false;
	}
	
	VkPipelineStageFlags flags = 
	{
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
	};
	
    VkSubmitInfo submit_info = 
	{
      VK_STRUCTURE_TYPE_SUBMIT_INFO,                        // VkStructureType                sType
      nullptr,                                              // const void                   * pNext
      1,   													// uint32_t                       waitSemaphoreCount
      &AcquiredSemaphore,                        			// const VkSemaphore            * pWaitSemaphores
      &flags,                   							// const VkPipelineStageFlags   * pWaitDstStageMask
      1,        											// uint32_t                       commandBufferCount
      &command_buffer,                               		// const VkCommandBuffer        * pCommandBuffers
      1,      												// uint32_t                       signalSemaphoreCount
      &ReadySemaphore                              			// const VkSemaphore            * pSignalSemaphores
    };
	
	if(!SubmitCommandBuffersToQueue(ComputeQueue, VK_NULL_HANDLE, submit_info))
	{
		return false;
	}
	
    VkPresentInfoKHR present_info = 
	{
      VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,                   // VkStructureType          sType
      nullptr,                                              // const void*              pNext
      1,   													// uint32_t                 waitSemaphoreCount
      &ReadySemaphore,                          			// const VkSemaphore      * pWaitSemaphores
      1,									             	// uint32_t                 swapchainCount
      &swapchain,                                    		// const VkSwapchainKHR   * pSwapchains
      &image_index,                                 			// const uint32_t         * pImageIndices
      nullptr                                               // VkResult*                pResults
    };
	
	result = vkQueuePresentKHR(ComputeQueue, &present_info);
    switch(result)
	{
		case VK_SUCCESS:
		  return true;
		default:
		  return false;
	}
	return true;
}

void mainLoop() 
{
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
	CreateRenderPass(depthFormat, false);
	CreateRenderPass(depthFormat, true);

    while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		if(!Draw())
		{
			std::cout << "Critical Error! Abandon the ship." << std::endl;
			break;
		}
        
    }
	DestroySwapchain(swapchain);
	DestroyPresentationSurface();
	glfwDestroyWindow(window);
}
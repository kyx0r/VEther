#include "window.h"
#include "control.h"
#include "render.h"
#include "shaders.h"

/* {
GVAR: framebufferCount -> render.cpp
GVAR: imageViewCount -> render.cpp
GVAR: imageViews -> render.cpp
GVAR: handle_array_of_swapchain_images -> swapchain.cpp
GVAR: number_of_swapchain_images -> swapchain.cpp
GVAR: logical_device -> startup.cpp
GVAR: pipeline_layout -> render.cpp
GVAR: pipelines -> render.h
} */

//{
GLFWwindow* _window;
VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
VkSwapchainKHR old_swapchain = VK_NULL_HANDLE;
VkSemaphore AcquiredSemaphore;
VkSemaphore ReadySemaphore;
VkFence Fence_one;
VkQueue GraphicsQueue;
VkQueue ComputeQueue;
int window_width = 1280;
int window_height = 960;
uint32_t graphics_queue_family_index; // <- this is queue 1
uint32_t compute_queue_family_index; // <- this is queue 2
//}

namespace window
{

void window_size_callback(GLFWwindow* _window, int width, int height)
{
	control::WaitForAllSubmittedCommandsToBeFinished();

	//handle minimization.
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(_window, &width, &height);
		glfwWaitEvents();
	}

	old_swapchain = _swapchain;
	_swapchain = VK_NULL_HANDLE;
	if(
	    !swapchain::SetSizeOfSwapchainImages(width, height)||
	    !swapchain::CreateSwapchain(_swapchain, old_swapchain)||
	    !swapchain::GetHandlesOfSwapchainImages(_swapchain)
	)
	{
		startup::debug_pause();
		exit(1);
	}
	window_height = height;
	window_width = width;

	render::DestroyImageViews();
	render::DestroyFramebuffers();
	imageViewCount = 0;
	framebufferCount = 0;
	render::CreateImageViews(number_of_swapchain_images, &handle_array_of_swapchain_images[0], image_format, 0, 1);
	render::CreateFramebuffers(number_of_swapchain_images, &imageViews[0], &imageViews[0], 0, window_width, window_height);
}

void keyCallback(GLFWwindow* _window, int key, int scancode, int action, int mods)
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

	_window = glfwCreateWindow(window_width, window_height, "VEther", nullptr, nullptr);
	glfwSetWindowSizeCallback(_window, window_size_callback);
	glfwSetKeyCallback(_window, keyCallback);
}

inline bool Draw()
{
	uint32_t image_index;
	VkResult result;
	control::WaitForAllSubmittedCommandsToBeFinished();
	vkWaitForFences(logical_device, 1, &Fence_one, VK_TRUE, std::numeric_limits<uint64_t>::max());
	if(
	    !swapchain::AcquireSwapchainImage(_swapchain, AcquiredSemaphore, VK_NULL_HANDLE, image_index)||
	    !control::BeginCommandBufferRecordingOperation(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr)
	)
	{
		return false;
	}

	VkClearColorValue color =
	{
		0,
		0,
		0,
		1
	};

	VkClearValue clearColor = {};
	clearColor.color = color;

	VkRect2D render_area = {};
	render_area.extent.width = window_width;
	render_area.extent.height = window_height;

	VkImageSubresourceRange range =
	{
		VK_IMAGE_ASPECT_COLOR_BIT,                // VkImageAspectFlags         aspectMask
		0,                                        // uint32_t                   baseMipLevel
		VK_REMAINING_MIP_LEVELS,                  // uint32_t                   levelCount
		0,                                        // uint32_t                   baseArrayLayer
		VK_REMAINING_ARRAY_LAYERS                 // uint32_t                   layerCount
	};

	//vkCmdClearColorImage(command_buffer, handle_array_of_swapchain_images[image_index], VK_IMAGE_LAYOUT_GENERAL, &color, 1, &range);

	VkImageMemoryBarrier image_memory_barrier_before_draw =
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		nullptr,                                  	// const void               * pNext
		VK_ACCESS_MEMORY_READ_BIT,           		// VkAccessFlags              srcAccessMask
		0,       									// VkAccessFlags              dstAccessMask
		VK_IMAGE_LAYOUT_UNDEFINED,           		// VkImageLayout              oldLayout
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,   // VkImageLayout              newLayout
		VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                   srcQueueFamilyIndex
		VK_QUEUE_FAMILY_IGNORED,                   	// uint32_t                   dstQueueFamilyIndex
		handle_array_of_swapchain_images[image_index],   // VkImage                    image
		range
	};
	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                     0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier_before_draw);

	render::StartRenderPass(render_area, &clearColor, VK_SUBPASS_CONTENTS_INLINE, 0, image_index);

	VkViewport viewport = { 0, 0, float(window_width), float(window_height), 0, 1 };
	VkRect2D scissor = { {0, 0}, {uint32_t(window_width), uint32_t(window_height)} };

	vkCmdSetViewport(command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0]);
	vkCmdDraw(command_buffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(command_buffer);

	VkImageMemoryBarrier image_memory_barrier_before_present =
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		nullptr,                                  	// const void               * pNext
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,       // VkAccessFlags              srcAccessMask
		VK_ACCESS_MEMORY_READ_BIT,       			// VkAccessFlags              dstAccessMask
		VK_IMAGE_LAYOUT_UNDEFINED,           		// VkImageLayout              oldLayout
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,   			// VkImageLayout              newLayout
		VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                   srcQueueFamilyIndex
		VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                   dstQueueFamilyIndex
		handle_array_of_swapchain_images[image_index],   // VkImage                    image
		range
	};
	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	                     0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier_before_present);

	if(!control::EndCommandBufferRecordingOperation())
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

	vkResetFences(logical_device, 1, &Fence_one);
	if(!control::SubmitCommandBuffersToQueue(ComputeQueue, Fence_one, submit_info))
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
		&_swapchain,                                    		// const VkSwapchainKHR   * pSwapchains
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
	double time = glfwGetTime()/1000;

	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
	render::CreateRenderPasses(1,depthFormat, false);

	render::CreateImageViews(number_of_swapchain_images, &handle_array_of_swapchain_images[0], image_format, 0, 1);
	render::CreateFramebuffers(number_of_swapchain_images, &imageViews[0], &imageViews[0], 0, window_width, window_height);

	VkShaderModule triangleVS = shaders::loadShader("res/shaders/triangle.vert.spv");
	assert(triangleVS);

	VkShaderModule triangleFS = shaders::loadShader("res/shaders/triangle.frag.spv");
	assert(triangleFS);

	VkPipelineCache pipelineCache = 0;

	render::CreatePipelineLayout();

	render::CreateGraphicsPipelines(2, pipelineCache, 0, triangleVS, triangleFS);

	while (!glfwWindowShouldClose(_window))
	{
		glfwPollEvents();
		if(!Draw())
		{
			std::cout << "Critical Error! Abandon the ship." << std::endl;
			break;
		}
	}

	//CLEANUP -----------------------------------
	control::WaitForAllSubmittedCommandsToBeFinished();
	control::FreeCommandBuffers(1);
	control::DestroyCommandPool();
	vkDestroySemaphore(logical_device, AcquiredSemaphore, nullptr);
	vkDestroySemaphore(logical_device, ReadySemaphore, nullptr);
	vkDestroyFence(logical_device, Fence_one, nullptr);
	render::DestroyImageViews();
	render::DestroyFramebuffers();
	vkDestroyPipelineLayout(logical_device, pipeline_layout, nullptr);
	render::DestroyPipeLines();
	vkDestroyShaderModule(logical_device, triangleVS, nullptr);
	vkDestroyShaderModule(logical_device, triangleFS, nullptr);
	render::DestroyRenderPasses();
	swapchain::DestroySwapchain(_swapchain);
	surface::DestroyPresentationSurface();
	glfwDestroyWindow(_window);
}

} //namespace window
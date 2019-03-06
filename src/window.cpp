#include "window.h"
#include "control.h"
#include "render.h"
#include "shaders.h"
#include "draw.h"

/* {
GVAR: framebufferCount -> render.cpp
GVAR: imageViewCount -> render.cpp
GVAR: imageViews -> render.cpp
GVAR: handle_array_of_swapchain_images -> swapchain.cpp
GVAR: number_of_swapchain_images -> swapchain.cpp
GVAR: logical_device -> startup.cpp
GVAR: pipeline_layout -> render.cpp
GVAR: pipelines -> render.h
GVAR: dyn_vertex_buffers -> control.cpp
GVAR: current_dyn_buffer_index -> control.cpp
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

	VkPipelineStageFlags flags =
	{
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
	};

	static bool once = true;
	static VkClearValue clearColor = {};
	static VkClearColorValue color = {};
	static VkImageSubresourceRange range = {};
	static VkImageMemoryBarrier image_memory_barrier_before_draw = {};
	static VkImageMemoryBarrier image_memory_barrier_before_present = {};
	static VkSubmitInfo submit_info = {};
	static VkPresentInfoKHR present_info = {};
	if(once)
	{
		color.float32[0] = 0;
		color.float32[1] = 0;
		color.float32[2] = 0;
		color.float32[3] = 1;
		clearColor.color = color;

		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.levelCount = VK_REMAINING_MIP_LEVELS;
		range.baseArrayLayer = 0;
		range.layerCount = VK_REMAINING_ARRAY_LAYERS;

		image_memory_barrier_before_draw.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier_before_draw.pNext = nullptr;
		image_memory_barrier_before_draw.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		image_memory_barrier_before_draw.dstAccessMask = 0;
		image_memory_barrier_before_draw.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_memory_barrier_before_draw.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		image_memory_barrier_before_draw.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier_before_draw.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier_before_draw.subresourceRange = range;

		image_memory_barrier_before_present.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier_before_present.pNext = nullptr;
		image_memory_barrier_before_present.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		image_memory_barrier_before_present.dstAccessMask = 0;
		image_memory_barrier_before_present.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_memory_barrier_before_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		image_memory_barrier_before_present.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier_before_present.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier_before_present.subresourceRange = range;

		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.pNext = nullptr;
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &AcquiredSemaphore;
		submit_info.pWaitDstStageMask = &flags;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer;
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &ReadySemaphore;

		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.pNext = nullptr;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = &ReadySemaphore;
		present_info.swapchainCount = 1;
		present_info.pSwapchains = &_swapchain;
		present_info.pImageIndices = &image_index;
		present_info.pResults = nullptr;

		once = false;
	}

	VkRect2D render_area = {};
	render_area.extent.width = window_width;
	render_area.extent.height = window_height;

	image_memory_barrier_before_draw.image = handle_array_of_swapchain_images[image_index];

	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                     0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier_before_draw);

	render::StartRenderPass(render_area, &clearColor, VK_SUBPASS_CONTENTS_INLINE, 0, image_index);

	VkViewport viewport = { 0, 0, float(window_width), float(window_height), 0, 1 };
	VkRect2D scissor = { {0, 0}, {uint32_t(window_width), uint32_t(window_height)} };

	vkCmdSetViewport(command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	control::InvalidateDynamicBuffers();

	current_dyn_buffer_index = (current_dyn_buffer_index + 1) % NUM_DYNAMIC_BUFFERS;
	static Vertex vertices[3] = {};
	static float x1 = 0.5f;
	static float y1 = -0.5f;
	static float x2 = 0.6f;
	static float y2 = 0.0f;
	static float col = 0;
	static bool colv = true;

	static float arr[4][4] =
	{
		{x1,y1,x2,y2},
		{x1,-y1,x2,-y2},
		{-x1,y1,-x2,y2},
		{-x1,-y1,-x2,-y2},
	};

	for(int i = 0; i<4; i++)
	{
		// x, y

		//   *
		// *   *

		vertices[0].pos = {arr[i][0], arr[i][1]};
		vertices[1].pos = {arr[i][2], arr[i][3]};
		vertices[2].pos = {0, 0};
		if(colv)
		{
			col = col + 0.01f;
			if(col > 10)
			{
				colv = false;
			}
		}
		else
		{
			col = col - 0.01f;
			if(col <= 0.01f)
			{
				colv = true;
			}
		}
		vertices[0].color = {col, 0.0f, 0.0f};
		vertices[1].color = {0.0f, col, 0.0f};
		vertices[2].color = {0.0f, 0.0f, col};
		draw::Draw_Triangle(sizeof(vertices[0]) * ARRAYSIZE(vertices), &vertices[0]);
	}

	vkCmdEndRenderPass(command_buffer);

	image_memory_barrier_before_present.image = handle_array_of_swapchain_images[image_index];

	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	                     0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier_before_present);

	control::FlushDynamicBuffers();

	if(!control::EndCommandBufferRecordingOperation())
	{
		return false;
	}

	vkResetFences(logical_device, 1, &Fence_one);
	if(!control::SubmitCommandBuffersToQueue(ComputeQueue, Fence_one, submit_info))
	{
		return false;
	}

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

	double time1 = 0;
	double time2 = 0;
	double time_diff = 0;

	while (!glfwWindowShouldClose(_window))
	{
		glfwPollEvents();
		time1 = glfwGetTime();
		time_diff = time1 - time2;
		if(time_diff > 0.01f)
		{
			if(!Draw())
			{
				std::cout << "Critical Error! Abandon the ship." << std::endl;
				break;
			}
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds((int)(time_diff*1000)));
			continue;
		}
		time2 = glfwGetTime();
	}

	//CLEANUP -----------------------------------
	control::WaitForAllSubmittedCommandsToBeFinished();
	control::FreeCommandBuffers(1);
	control::DestroyCommandPool();
	control::DestroyDynBuffers();
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
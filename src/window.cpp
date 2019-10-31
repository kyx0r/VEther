#include "window.h"
#include "control.h"
#include "shaders.h"
#include "textures.h"
#include "draw.h"
#include "obj_parse.h"
#include "flog.h"
#include "entity.h"

/* {
GVAR: framebufferCount -> render.cpp
GVAR: imageViewCount -> render.cpp
GVAR: imageViews -> render.cpp
GVAR: number_of_swapchain_images -> swapchain.cpp
GVAR: logical_device -> startup.cpp
GVAR: pipeline_layout -> render.cpp
GVAR: pipelines -> render.h
GVAR: dyn_vertex_buffers -> control.cpp
GVAR: current_dyn_buffer_index -> control.cpp
GVAR: NUM_COMMAND_BUFFERS -> control.cpp
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
double time1 = 0;
double y_wheel = 0;
double xm_norm = 0;
double ym_norm = 0;
uint32_t xm = 0;
uint32_t ym = 0;
double deltatime = 0;
double frametime;
//}

static ParsedOBJ kitty;

namespace window
{

void window_size_callback(GLFWwindow* _window, int width, int height)
{
	//handle minimization.
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(_window, &width, &height);
		glfwWaitEvents();
	}

	VK_CHECK(vkDeviceWaitIdle(logical_device));

	old_swapchain = _swapchain;
	_swapchain = VK_NULL_HANDLE;
	if(
	    !surface::CheckPresentationSurfaceCapabilities()||
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

	for(uint32_t i = 0; i<number_of_swapchain_images; i++)
	{
		vkDestroyImageView(logical_device, imageViews[i], nullptr);
	}
	uint32_t tmpc = imageViewCount;
	imageViewCount = 0;
	render::CreateSwapchainImageViews();

	render::DestroyFramebuffers();
	framebufferCount = 0;

	render::CreateDepthBuffer();
	imageViewCount = tmpc;

	for(uint16_t i = 0; i<number_of_swapchain_images; i++)
	{
		render::CreateFramebuffer(&imageViews[i], &imageViews[number_of_swapchain_images], 0, window_width, window_height);
	}
}

void keyCallback(GLFWwindow* _window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if(key == GLFW_KEY_ESCAPE)
		{
			trace("Quiting cleanly");
			glfwSetWindowShouldClose(_window, true);
		}
	}
}

void processInput()
{
	float velocity = cam.speed * frametime;
	if (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS)
	{
		//camera.ProcessKeyboard(FORWARD, deltaTime);
		cam.pos[0] += cam.front[0] * velocity;
		cam.pos[1] += cam.front[1] * velocity;
		cam.pos[2] += cam.front[2] * velocity;
	}
	if (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS)
	{
		cam.pos[0] -= cam.front[0] * velocity;
		cam.pos[1] -= cam.front[1] * velocity;
		cam.pos[2] -= cam.front[2] * velocity;

		//camera.ProcessKeyboard(BACKWARD, deltaTime);
	}
	if (glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS)
	{
		cam.pos[0] -= cam.right[0] * velocity;
		cam.pos[1] -= cam.right[1] * velocity;
		cam.pos[2] -= cam.right[2] * velocity;

		//camera.ProcessKeyboard(LEFT, deltaTime);
	}
	if (glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS)
	{
		cam.pos[0] += cam.right[0] * velocity;
		cam.pos[1] += cam.right[1] * velocity;
		cam.pos[2] += cam.right[2] * velocity;

		// camera.ProcessKeyboard(RIGHT, deltaTime);
	}
}

float lastX = window_height / 2.0f;
float lastY = window_width / 2.0f;


void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	//  printf("Xpos: %.6f \n", xpos);
	//printf("Ypos: %.6f \n", ypos);
	xm_norm = (xpos - ((window_width - 0.001) / 2)) / ((window_width - 0.001) / 2);
	ym_norm = (ypos - ((window_height - 0.001) / 2)) / ((window_height - 0.001) / 2);
	xm = (uint32_t)xpos;
	ym = (uint32_t)ypos;
	//printf("Xpos: %.6f \n", x);
	//printf("Ypos: %.6f \n", y);

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	xoffset *= cam.sensitivity;
	yoffset *= cam.sensitivity;

	cam.yaw   += xoffset;
	cam.pitch -= yoffset;

	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (cam.pitch > 89.0f)
		cam.pitch = 89.0f;
	if (cam.pitch < -89.0f)
		cam.pitch = -89.0f;
	entity::UpdateCamera();
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	y_wheel += yoffset;
	if (cam.zoom >= 1.0f && cam.zoom <= 45.0f)
		cam.zoom -= yoffset;
	if (cam.zoom <= 1.0f)
		cam.zoom = 1.0f;
	if (cam.zoom >= 45.0f)
		cam.zoom = 45.0f;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{

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
	glfwSetCursorPosCallback(_window, cursor_position_callback);
	glfwSetScrollCallback(_window, scroll_callback);
	glfwSetMouseButtonCallback(_window, mouse_button_callback);
	glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

inline uint8_t Draw()
{
	uint32_t image_index;
	VkResult result;

	vkWaitForFences(logical_device, 1, &Fence_one, VK_TRUE, UINT64_MAX);
	vkResetFences(logical_device, 1, &Fence_one);
	uint8_t ret = swapchain::AcquireSwapchainImage(_swapchain, AcquiredSemaphore, VK_NULL_HANDLE, image_index);

	while(ret == 2)
	{
		ret = swapchain::AcquireSwapchainImage(_swapchain, AcquiredSemaphore, VK_NULL_HANDLE, image_index);
		glfwPollEvents();
	}
	if(!ret)
	{
		return 0;
	}

	assert(control::BeginCommandBufferRecordingOperation(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr));

	VkPipelineStageFlags flags =
	{
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
	};

	static bool once = true;
	static VkClearValue clearColor[2] = {};
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
		clearColor[0].color = color;
		clearColor[1].depthStencil = {1.0f, 0};

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

	render::StartRenderPass(render_area, &clearColor[0], VK_SUBPASS_CONTENTS_INLINE, 0, image_index);

	VkViewport viewport = {0, 0, float(window_width), float(window_height), 0, 1 };
	VkRect2D scissor = { {0, 0}, {uint32_t(window_width), uint32_t(window_height)} };

	vkCmdSetViewport(command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	processInput();
	entity::UpdateCamera();
	float m[16];
	float c[16];
	//SetupMatrix(m);
	//IdentityMatrix(m);
	//FrustumMatrix(m, DEG2RAD(fovx), DEG2RAD(fovy));
	Perspective(m, DEG2RAD(cam.zoom), float(window_width) / float(window_height), 0.1f, 100.0f); //projection matrix.
	entity::ViewMatrix(c);
	//PrintMatrix(c);
	MatrixMultiply(m, c);
	vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, 16 * sizeof(float), &m);

	//	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[1]);
	//vkCmdDraw(command_buffer, 6, 1, 0, 0);

	ParsedOBJSubModel p = *kitty.models->sub_models;
	draw::DrawIndexedTriangle(32 * p.vertex_count, (Vertex_*)p.vertices, p.index_count, (uint32_t*)p.indices);

	vkCmdEndRenderPass(command_buffer);

	image_memory_barrier_before_present.image = handle_array_of_swapchain_images[image_index];

	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	                     0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier_before_present);

	if(!control::EndCommandBufferRecordingOperation())
	{
		return 0;
	}

	VK_CHECK(vkQueueSubmit(GraphicsQueue, 1, &submit_info, Fence_one));

	result = vkQueuePresentKHR(GraphicsQueue, &present_info);
	switch(result)
	{
	case VK_SUCCESS:
		return 1;
	case VK_ERROR_OUT_OF_DATE_KHR:
		glfwPollEvents();
		return 1;
	default:
		printf(startup::GetVulkanResultString(result));
		printf("\n");
		return 0;
	}
	return 1;
}

void mainLoop()
{
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
	render::CreateRenderPass(depthFormat, false);
	render::CreateSwapchainImageViews();

	render::CreateDepthBuffer();

	for(uint16_t i = 0; i<number_of_swapchain_images; i++)
	{
		render::CreateFramebuffer(&imageViews[i], &imageViews[number_of_swapchain_images], 0, window_width, window_height);
	}

	shaders::CompileShaders();

	VkShaderModule triangleFS = shaders::loadShaderMem(0);
	ASSERT(triangleFS, "Failed to load triangleFS.");
	VkShaderModule triangleVS = shaders::loadShaderMem(1);
	ASSERT(triangleVS, "Failed to load triangleVS.");
	VkShaderModule screenFS = shaders::loadShaderMem(2);
	ASSERT(triangleVS, "Failed to load screenFS.");
	VkShaderModule screenVS = shaders::loadShaderMem(3);
	ASSERT(triangleVS, "Failed to load screenVS.");


	textures::SampleTexture();

	kitty = LoadOBJ("./res/kitty.obj");

	VkPipelineCache pipelineCache = 0;

	render::CreatePipelineLayout();

	render::CreateGraphicsPipeline(pipelineCache, render::BasicTrianglePipe, 0, triangleVS, triangleFS);

	render::CreateGraphicsPipeline(pipelineCache, render::ScreenPipe, 0, screenVS, screenFS);

	//fov setup.
	entity::InitCamera();

	double time2 = 0;
	double maxfps;
	double realtime = 0;
	double oldrealtime = 0;

	while (!glfwWindowShouldClose(_window))
	{
		glfwPollEvents();
		time1 = glfwGetTime();
		deltatime = time1 - time2;
		realtime += deltatime;
		maxfps = CLAMP (10.0, 60.0, 1000.0); //60 fps

		if(realtime - oldrealtime < 1.0/maxfps)
		{
			goto c; //framerate is too high
		}
		frametime = realtime - oldrealtime;
		oldrealtime = realtime;
		frametime = CLAMP (0.0001, frametime, 0.1);

		if(!Draw())
		{
			fatal("Critical Error! Abandon the ship.");
			break;
		}

c:
		;
		if(deltatime < 0.02f)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		time2 = time1;
	}

	//CLEANUP -----------------------------------
	VK_CHECK(vkDeviceWaitIdle(logical_device));
	control::FreeCommandBuffers(NUM_COMMAND_BUFFERS);
	control::DestroyCommandPool();
	control::DestroyDynBuffers();
	control::DestroyStagingBuffers();
	control::DestroyUniformBuffers();
	control::DestroyIndexBuffers();
	control::DestroyVramHeaps();
	textures::TexDeinit();
	vkDestroySemaphore(logical_device, AcquiredSemaphore, nullptr);
	vkDestroySemaphore(logical_device, ReadySemaphore, nullptr);
	vkDestroyFence(logical_device, Fence_one, nullptr);
	render::DestroyImageViews();
	render::DestroyFramebuffers();
	vkDestroyPipelineLayout(logical_device, pipeline_layout, nullptr);
	render::DestroyPipeLines();
	vkDestroyShaderModule(logical_device, triangleVS, nullptr);
	vkDestroyShaderModule(logical_device, triangleFS, nullptr);
	vkDestroyShaderModule(logical_device, screenVS, nullptr);
	vkDestroyShaderModule(logical_device, screenFS, nullptr);
	render::DestroyRenderPasses();
	swapchain::DestroySwapchain(_swapchain);
	surface::DestroyPresentationSurface();
	glfwDestroyWindow(_window);
}

} //namespace window

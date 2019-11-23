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
GVAR: allocators -> startup.cpp
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
double frametime;
mu_Context* ctx;
//}

static ParsedOBJ kitty;
static int mu_eidx;

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
		if(key == GLFW_KEY_M)
		{
			static bool state = true;
			if(state)
			{
				info("Mouse focused.");
				glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				state = false;
			}
			else
			{
				info("Default mouse.");
				glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				state = true;
			}
		}

	}
}

void processInput()
{
	float velocity = cam.speed * frametime;
	if (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS)
	{
		cam.pos[0] += cam.front[0] * velocity;
		cam.pos[1] += cam.front[1] * velocity;
		cam.pos[2] += cam.front[2] * velocity;
	}
	if (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS)
	{
		cam.pos[0] -= cam.front[0] * velocity;
		cam.pos[1] -= cam.front[1] * velocity;
		cam.pos[2] -= cam.front[2] * velocity;
	}
	if (glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS)
	{
		cam.pos[0] -= cam.right[0] * velocity;
		cam.pos[1] -= cam.right[1] * velocity;
		cam.pos[2] -= cam.right[2] * velocity;
	}
	if (glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS)
	{
		cam.pos[0] += cam.right[0] * velocity;
		cam.pos[1] += cam.right[1] * velocity;
		cam.pos[2] += cam.right[2] * velocity;
	}
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	static float lastX = window_height / 2.0f;
	static float lastY = window_width / 2.0f;
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
}

static  char logbuf[64000];
static   int logbuf_updated = 0;

static void write_log(const char *text)
{
	if (logbuf[0])
	{
		strcat(logbuf, "\n");
	}
	strcat(logbuf, text);
	logbuf_updated = 1;
}

static void log_window(mu_Context *ctx)
{
	static mu_Container window;

	/* init window manually so we can set its position and size */
	if (!window.inited)
	{
		mu_init_window(ctx, &window, 0);
		window.rect = mu_rect(350, 40, 300, 200);
	}

	if (mu_begin_window(ctx, &window, "Log Window"))
	{

		/* output text panel */
		static mu_Container panel;
		int width[] = {-1};
		mu_layout_row(ctx, 1, width, -28);
		mu_begin_panel(ctx, &panel);
		mu_layout_row(ctx, 1, width, -1);
		mu_text(ctx, logbuf);
		mu_end_panel(ctx);
		if (logbuf_updated)
		{
			panel.scroll.y = panel.content_size.y;
			logbuf_updated = 0;
		}

		/* input textbox + submit button */
		static char buf[128];
		int submitted = 0;
		int _width[] = { -70, -1 };
		mu_layout_row(ctx, 2, _width, 0);
		if (mu_textbox(ctx, buf, sizeof(buf)) & MU_RES_SUBMIT)
		{
			mu_set_focus(ctx, ctx->last_id);
			submitted = 1;
		}
		if (mu_button(ctx, "Submit"))
		{
			submitted = 1;
		}
		if (submitted)
		{
			write_log(buf);
			buf[0] = '\0';
		}

		mu_end_window(ctx);
	}
}

static int uint8_slider(mu_Context *ctx, unsigned char *value, int low, int high)
{
	static float tmp;
	mu_push_id(ctx, &value, sizeof(value));
	tmp = *value;
	int res = mu_slider_ex(ctx, &tmp, low, high, 0, "%.0f", MU_OPT_ALIGNCENTER);
	*value = tmp;
	mu_pop_id(ctx);
	return res;
}

static void style_window(mu_Context *ctx)
{
	static mu_Container window;

	/* init window manually so we can set its position and size */
	if (!window.inited)
	{
		mu_init_window(ctx, &window, 0);
		window.rect = mu_rect(350, 250, 300, 240);
	}

	static struct
	{
		const char *label;
		int idx;
	} colors[] =
	{
		{ "text:",         MU_COLOR_TEXT        },
		{ "border:",       MU_COLOR_BORDER      },
		{ "windowbg:",     MU_COLOR_WINDOWBG    },
		{ "titlebg:",      MU_COLOR_TITLEBG     },
		{ "titletext:",    MU_COLOR_TITLETEXT   },
		{ "panelbg:",      MU_COLOR_PANELBG     },
		{ "button:",       MU_COLOR_BUTTON      },
		{ "buttonhover:",  MU_COLOR_BUTTONHOVER },
		{ "buttonfocus:",  MU_COLOR_BUTTONFOCUS },
		{ "base:",         MU_COLOR_BASE        },
		{ "basehover:",    MU_COLOR_BASEHOVER   },
		{ "basefocus:",    MU_COLOR_BASEFOCUS   },
		{ "scrollbase:",   MU_COLOR_SCROLLBASE  },
		{ "scrollthumb:",  MU_COLOR_SCROLLTHUMB },
		{ NULL, 0 }
	};

	if (mu_begin_window(ctx, &window, "Style Editor"))
	{
		int sw = mu_get_container(ctx)->body.w * 0.14;
		int widths[] = { 80, sw, sw, sw, sw, -1 };
		mu_layout_row(ctx, 6, widths, 0);
		for (int i = 0; colors[i].label; i++)
		{
			mu_label(ctx, colors[i].label);
			uint8_slider(ctx, &ctx->style->colors[i].r, 0, 255);
			uint8_slider(ctx, &ctx->style->colors[i].g, 0, 255);
			uint8_slider(ctx, &ctx->style->colors[i].b, 0, 255);
			uint8_slider(ctx, &ctx->style->colors[i].a, 0, 255);
			mu_draw_rect(ctx, mu_layout_next(ctx), ctx->style->colors[i]);
		}
		mu_end_window(ctx);
	}
}


inline uint8_t Draw()
{
	uint32_t image_index;
	VkResult result;
	mu_Command* cmd = nullptr;

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

		mu_eidx = draw::buf_idx;

		once = false;
	}

	mu_begin(ctx);
	log_window(ctx);
	style_window(ctx);
	mu_end(ctx);

	//record ui commands.
	while (mu_next_command(ctx, &cmd))
	{
		switch (cmd->type)
		{
		case MU_COMMAND_TEXT:
			draw::r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color);
			break;
		case MU_COMMAND_RECT:
			draw::r_draw_rect(cmd->rect.rect, cmd->rect.color);
			break;
			// case MU_COMMAND_ICON: r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
			// case MU_COMMAND_CLIP: r_set_clip_rect(cmd->clip.rect); break;
		}
		//break;
	}
	draw::buf_idx = mu_eidx;

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
	vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), &m);
	vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 16 * sizeof(float), sizeof(uint32_t), &window_width);
	vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 16 * sizeof(float) + sizeof(uint32_t), sizeof(uint32_t), &window_height);

	draw::PresentUI();

	// static Uivertex vertices[4] = {};

	// vertices[0].pos[0] = 300;   //x
	// vertices[0].pos[1] = 200;   //y
	// vertices[0].pos[2] = 0.0f;   //z
	// vertices[0].color = 0xFFFFFFFF;
	// vertices[0].tex_coord[0] = 1.0f;
	// vertices[0].tex_coord[1] = 0.0f;

	// vertices[1].pos[0] = 400;
	// vertices[1].pos[1] = 200;
	// vertices[1].pos[2] = 0.0f;
	// vertices[1].color = 0xFFFFFFFF;
	// vertices[1].tex_coord[0] = 0.0f;
	// vertices[1].tex_coord[1] = 0.0f;

	// vertices[2].pos[0] = 300;
	// vertices[2].pos[1] = 100;
	// vertices[2].pos[2] = 0.0f;
	// vertices[2].color = 0xFFFFFFFF;
	// vertices[2].tex_coord[0] = 0.0f;
	// vertices[2].tex_coord[1] = 1.0f;

	// vertices[3].pos[0] = 400;
	// vertices[3].pos[1] = 100;
	// vertices[3].pos[2] = 0.0f;
	// vertices[3].color = 0xFFFFFFFF;
	// vertices[3].tex_coord[0] = 1.0f;
	// vertices[3].tex_coord[1] = 1.0f;

	// uint16_t indeces[6] = {0, 1, 2, 2, 3, 1};

	// draw::DrawQuad(sizeof(vertices[0]) * ARRAYSIZE(vertices), &vertices[0], ARRAYSIZE(indeces), indeces);

	// static Vertex_ vertices[4] = {};

	// vertices[0].pos[0] = -0.5f;   //x
	// vertices[0].pos[1] = -0.5f;   //y
	// vertices[0].pos[2] = 0.0f;   //z
	// vertices[0].color[0] = 1.0f; //r
	// vertices[0].color[1] = 0.0f; //g
	// vertices[0].color[2] = 0.0f; //b
	// vertices[0].tex_coord[0] = 1.0f;
	// vertices[0].tex_coord[1] = 0.0f;

	// vertices[1].pos[0] = 0.5f;
	// vertices[1].pos[1] = -0.5f;
	// vertices[1].pos[2] = 0.0f;
	// vertices[1].color[0] = 0.0f;
	// vertices[1].color[1] = 1.0f;
	// vertices[1].color[2] = 0.0f;
	// vertices[1].tex_coord[0] = 0.0f;
	// vertices[1].tex_coord[1] = 0.0f;

	// vertices[2].pos[0] = 0.5f;
	// vertices[2].pos[1] = 0.5f;
	// vertices[2].pos[2] = 0.0f;
	// vertices[2].color[0] = 0.0f;
	// vertices[2].color[1] = 0.0f;
	// vertices[2].color[2] = 1.0f;
	// vertices[2].tex_coord[0] = 0.0f;
	// vertices[2].tex_coord[1] = 1.0f;

	// vertices[3].pos[0] = -0.5f;
	// vertices[3].pos[1] = 0.5f;
	// vertices[3].pos[2] = 0.0f;
	// vertices[3].color[0] = 1.0f;
	// vertices[3].color[1] = 1.0f;
	// vertices[3].color[2] = 1.0f;
	// vertices[3].tex_coord[0] = 1.0f;
	// vertices[3].tex_coord[1] = 1.0f;

	// uint16_t indeces[6] = {0, 1, 2, 2, 3, 0};

	// draw::DrawQuad(sizeof(vertices[0]) * ARRAYSIZE(vertices), &vertices[0], ARRAYSIZE(indeces), indeces);

	//vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[1]);
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

	control::ResetStagingBuffer();
	//textures::SampleTextureUpdate();

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

	//textures::SampleTexture();

	kitty = LoadOBJ("./res/kitty.obj");

	VkPipelineCache pipelineCache = 0;

	render::CreatePipelineLayout();

	render::CreateGraphicsPipeline(pipelineCache, render::BasicTrianglePipe, 0, triangleVS, triangleFS);

	render::CreateGraphicsPipeline(pipelineCache, render::ScreenPipe, 0, screenVS, screenFS);

	//fov setup.
	entity::InitCamera();

	/* init microui */
	ctx = (mu_Context*) zone::Hunk_Alloc(sizeof(mu_Context));
	mu_init(ctx);
	ctx->text_width = draw::text_width;
	ctx->text_height = draw::text_height;

	draw::InitAtlasTexture();

	double time2 = 0;
	double maxfps;
	double realtime = 0;
	double oldrealtime = 0;
	double deltatime = 0;

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
	render::DestroyDepthBuffer();
	control::DestroyCommandPool();
	control::DestroyDynBuffers();
	control::DestroyStagingBuffers();
	control::DestroyUniformBuffers();
	control::DestroyIndexBuffers();
	control::DestroyVramHeaps();
	textures::TexDeinit();
	vkDestroySemaphore(logical_device, AcquiredSemaphore, &allocators);
	vkDestroySemaphore(logical_device, ReadySemaphore, &allocators);
	vkDestroyFence(logical_device, Fence_one, &allocators);
	render::DestroyImageViews();
	render::DestroyFramebuffers();
	vkDestroyPipelineLayout(logical_device, pipeline_layout, &allocators);
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

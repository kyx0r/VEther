#include "startup.h"
#include "swapchain.h"
#include "zone.h"

//GVAR: command_buffer -> control.cpp

extern VkRenderPass renderPasses[10];
extern VkFramebuffer framebuffers[10];
extern VkImageView imageViews[100];
extern VkPipeline pipelines[20];

extern uint32_t renderPassCount;
extern uint32_t framebufferCount;
extern uint32_t imageViewCount;
extern uint32_t pipelineCount;

extern VkPipelineLayout pipeline_layout;
extern VkPipeline current_pipeline;

typedef struct
{
	float pos[3];   // = vec3
	float tex_coord[2]; // = vec2
	float color[3]; // = vec3
} Vertex_;

typedef struct
{
	float	model[16];
	float	view[16];
	float	proj[16];
} UniformMatrix;

namespace render
{

//-----------------------
void CreateFramebuffer(VkImageView *colorView, VkImageView *depthView, int render_index, uint32_t width, uint32_t height);
void CreateRenderPass(VkFormat depthFormat, bool late);
void CreateSwapchainImageViews();
void StartRenderPass(VkRect2D render_area, VkClearValue *clear_values, VkSubpassContents subpass_contents, int render_index, int buffer_index);
void CreateGraphicsPipeline(VkPipelineCache pipelineCache, VkPipelineVertexInputStateCreateInfo* (*vertexInput)(), int render_index, VkShaderModule vs, VkShaderModule fs);
void CreatePipelineLayout();
void CreateDepthBuffer();
void Viewport(float x, float y, float width, float height, float min_depth, float max_depth);
//-----------------------
VkPipelineVertexInputStateCreateInfo* BasicTrianglePipe();
VkImage Create2DImage(VkFormat format, VkImageUsageFlags usage, int w, int h);

//----------------------- inline

inline void DestroyRenderPasses()
{
	for(uint32_t i = 0; i<renderPassCount; i++)
		vkDestroyRenderPass(logical_device, renderPasses[i], nullptr);
}

inline void DestroyImageViews()
{
	for(uint32_t i = 0; i<imageViewCount; i++)
		vkDestroyImageView(logical_device, imageViews[i], nullptr);
}

inline void DestroyFramebuffers()
{
	for(uint32_t i = 0; i<framebufferCount; i++)
		vkDestroyFramebuffer(logical_device, framebuffers[i], nullptr);
}

inline void DestroyPipeLines()
{
	for(uint32_t i = 0; i<pipelineCount; i++)
		vkDestroyPipeline(logical_device, pipelines[i], nullptr);
}

} // namespace render

//

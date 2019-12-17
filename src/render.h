
#ifndef __RENDER__
#define __RENDER__

#include "startup.h"
#include "swapchain.h"
#include "zone.h"
#include "cvar.h"
#include "shaders.h"
//GVAR: command_buffer -> control.cpp

extern VkRenderPass renderPasses[10];
extern VkFramebuffer framebuffers[10];
extern VkImageView imageViews[100];
extern VkPipeline pipelines[20];

extern uint32_t renderPassCount;
extern uint32_t framebufferCount;
extern uint32_t imageViewCount;
extern uint32_t pipelineCount;

extern VkPipelineLayout pipeline_layout[20];
extern VkPipeline current_pipeline;

//These structs define renderables. Ie used in pipeline creation and in shaders.
typedef struct
{
	float x, y, z; //vec3
} Vertex;

typedef struct
{
	float pos[4];   // = vec4
} float4_t;

typedef struct
{
	float pos[3];   // = vec3
} float3_t;

typedef struct
{
	float pos[3];   // = vec3
	float tex_coord[2]; // = vec2
	float color[3]; // = vec3
} Vertex_;

typedef struct
{
	float pos[3];   // = vec3
	float tex_coord[2]; // = vec2
	uint32_t color;
} Uivertex;

typedef struct
{
	float	model[16];
	float	view[16];
	float	proj[16];
} UniformMatrix;

typedef struct
{
	float SkyColor[4];   // = vec4
	float almoshereColor[4];   // = vec4
	float groundColor[4];   // = vec4
	float atmosphereHeight;
} UniformSkydome;

namespace render
{

//-----------------------
void CreateFramebuffer(VkImageView *colorView, VkImageView *depthView, int render_index, uint32_t width, uint32_t height);
void CreateRenderPass(VkFormat depthFormat, bool late);
void CreateSwapchainImageViews();
void StartRenderPass(VkRect2D render_area, VkClearValue *clear_values, VkSubpassContents subpass_contents, int render_index, int buffer_index);
void CreateGraphicsPipeline(VkPipelineCache pipelineCache, VkPipelineVertexInputStateCreateInfo* (*vertexInput)(), int render_index, VkShaderModule vs, VkShaderModule fs);
void CreateTessGraphicsPipeline
(VkPipelineCache pipelineCache, VkPipelineVertexInputStateCreateInfo* (*vertexInput)(),
 int render_index, VkShaderModule vs, VkShaderModule fs, VkShaderModule cs, VkShaderModule es);
void CreatePipelineLayout();
void DestroyDepthBuffer();
void CreateDepthBuffer();
void Viewport(float x, float y, float width, float height, float min_depth, float max_depth);
//-----------------------
VkPipelineVertexInputStateCreateInfo* BasicTrianglePipe();
VkPipelineVertexInputStateCreateInfo* ScreenPipe();
VkPipelineVertexInputStateCreateInfo* Vec4FloatPipe();
VkPipelineVertexInputStateCreateInfo* Vec3FloatPipe();
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

//---------------------- callback
void RebuildPipelines(struct cvar_s*);

} // namespace render

#endif
//

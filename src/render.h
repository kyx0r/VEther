#include "startup.h"
#include "swapchain.h"
#include "glm/glm.hpp"

//GVAR: command_buffer -> control.cpp

extern VkRenderPass renderPasses[100];
extern VkFramebuffer framebuffers[100];
extern VkImageView imageViews[100];
extern VkPipeline pipelines[100];

extern uint32_t renderPassCount;
extern uint32_t framebufferCount;
extern uint32_t imageViewCount;
extern uint32_t pipelineCount;

extern VkPipelineLayout pipeline_layout;
extern VkPipeline current_pipeline;

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;
};

namespace render
{

//-----------------------
void CreateFramebuffers(int count, VkImageView *colorView, VkImageView *depthView, int render_index, uint32_t width, uint32_t height);
void CreateRenderPasses(int count, VkFormat depthFormat, bool late);
void CreateImageViews(int count, VkImage *image, VkFormat imageformat, uint32_t mipLevel, uint32_t levelCount);
void StartRenderPass(VkRect2D render_area, VkClearValue *clear_values, VkSubpassContents subpass_contents, int render_index, int buffer_index);
void CreateGraphicsPipelines(int count, VkPipelineCache pipelineCache, int render_index, VkShaderModule vs, VkShaderModule fs);
void BindPipeline(VkPipeline pipeline);
//----------------------- return
//-----------------------

//----------------------- inline

inline void CreatePipelineLayout()
{
	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	VK_CHECK(vkCreatePipelineLayout(logical_device, &createInfo, 0, &pipeline_layout));
}

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
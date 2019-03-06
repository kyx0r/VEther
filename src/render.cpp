#include "render.h"
#include "control.h"
#include "window.h"

/* {
GVAR: logical_device -> startup.cpp
GVAR: command_buffer -> control.h
GVAR: image_format -> swapchain.cpp
GVAR: dyn_vertex_buffers -> control.cpp
} */

//{
VkRenderPass renderPasses[100] = {};
VkFramebuffer framebuffers[100] = {};
VkImageView imageViews[100] = {};
VkPipeline pipelines[100] = {};
uint32_t renderPassCount = 0;
uint32_t framebufferCount = 0;
uint32_t imageViewCount = 0;
uint32_t pipelineCount = 0;
VkPipelineLayout pipeline_layout = 0;
VkPipeline current_pipeline = 0;
//}

namespace render
{

void CreateRenderPasses(int count, VkFormat depthFormat, bool late)
{
	VkAttachmentDescription attachments[1] = {};
	attachments[0].format = image_format;
	attachments[0].flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = late ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachment = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkSubpassDescription subpass = {};
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachment;

	/* 	attachments[1].flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].format = depthFormat;
		attachments[1].loadOp = late ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = late ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		VkAttachmentReference depthAttachment = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
		subpass.pDepthStencilAttachment = &depthAttachment; */


	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
	createInfo.pAttachments = attachments;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;

	renderPassCount += count;
	if(renderPassCount < ARRAYSIZE(renderPasses))
	{
		for(int i = 0; i<count; i++)
		{
			VK_CHECK(vkCreateRenderPass(logical_device, &createInfo, 0, &renderPasses[i]));
		}
		return;
	}
	std::cout<<"Info: Run out of render passes. \n";
	renderPassCount = 0;
	vkDestroyRenderPass(logical_device, renderPasses[renderPassCount], nullptr);
	return;
}

void CreateFramebuffers(int count, VkImageView *colorView, VkImageView *depthView, int render_index, uint32_t width, uint32_t height)
{
	//VkImageView attachments[] = { colorView, depthView };
	//VkImageView attachments[] = { colorView };

	VkFramebufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	createInfo.renderPass = renderPasses[render_index];
	createInfo.width = width;
	createInfo.height = height;
	createInfo.layers = 1;

	framebufferCount += count;
	if(framebufferCount < ARRAYSIZE(framebuffers))
	{
		for(int i = 0; i<count; i++)
		{
			VkImageView attachments[] = { colorView[i] };
			createInfo.attachmentCount = ARRAYSIZE(attachments);
			createInfo.pAttachments = attachments;
			VK_CHECK(vkCreateFramebuffer(logical_device, &createInfo, 0, &framebuffers[i]));
		}
		return;
	}
	std::cout<<"Info: Run out of frame buffers. \n";
	framebufferCount = 0;
	vkDestroyFramebuffer(logical_device, framebuffers[framebufferCount], nullptr);
	return;
}

void CreateImageViews(int count, VkImage *image, VkFormat imageformat, uint32_t mipLevel, uint32_t levelCount)
{
	VkImageAspectFlags aspectMask = (imageformat == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = imageformat;
	createInfo.subresourceRange.aspectMask = aspectMask;
	createInfo.subresourceRange.baseMipLevel = mipLevel;
	createInfo.subresourceRange.levelCount = levelCount;
	createInfo.subresourceRange.layerCount = 1;

	imageViewCount += count;
	if(imageViewCount < ARRAYSIZE(imageViews))
	{
		for(int i = 0; i<count; i++)
		{
			createInfo.image = image[i];
			VK_CHECK(vkCreateImageView(logical_device, &createInfo, 0, &imageViews[i]));
		}
		return;
	}
	std::cout<<"Info: Run out of imageViews. \n";
	imageViewCount = 0;
	return;
}

void StartRenderPass(VkRect2D render_area, VkClearValue *clear_values, VkSubpassContents subpass_contents, int render_index, int buffer_index)
{
	VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.pNext = nullptr;
	render_pass_begin_info.renderPass = renderPasses[render_index];
	render_pass_begin_info.framebuffer = framebuffers[buffer_index];
	render_pass_begin_info.renderArea = render_area;
	render_pass_begin_info.clearValueCount = 1;
	render_pass_begin_info.pClearValues = clear_values;

	vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, subpass_contents);
}

void CreateGraphicsPipelines(int count, VkPipelineCache pipelineCache, int render_index, VkShaderModule vs, VkShaderModule fs)
{
	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	VkPipelineShaderStageCreateInfo stages[2] = {};
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = vs;
	stages[0].pName = "main";
	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = fs;
	stages[1].pName = "main";

	createInfo.stageCount = ARRAYSIZE(stages);
	createInfo.pStages = stages;

	VkPipelineVertexInputStateCreateInfo vertexInput = {};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	createInfo.pVertexInputState = &vertexInput;

	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attributeDescriptions[2];
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.vertexAttributeDescriptionCount = ARRAYSIZE(attributeDescriptions);
	vertexInput.pVertexBindingDescriptions = &bindingDescription;
	vertexInput.pVertexAttributeDescriptions = &attributeDescriptions[0];

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	createInfo.pInputAssemblyState = &inputAssembly;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	createInfo.pViewportState = &viewportState;

	VkPipelineRasterizationStateCreateInfo rasterizationState = {};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.lineWidth = 1.f;
	createInfo.pRasterizationState = &rasterizationState;

	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.pMultisampleState = &multisampleState;

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	createInfo.pDepthStencilState = &depthStencilState;

	VkPipelineColorBlendAttachmentState colorAttachmentState = {};
	colorAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &colorAttachmentState;
	createInfo.pColorBlendState = &colorBlendState;

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = ARRAYSIZE(dynamicStates);
	dynamicState.pDynamicStates = dynamicStates;
	createInfo.pDynamicState = &dynamicState;

	createInfo.layout = pipeline_layout;
	createInfo.renderPass = renderPasses[render_index];

	pipelineCount += count;
	if(pipelineCount < ARRAYSIZE(pipelines))
	{
		for(int i = 0; i<count; i++)
		{
			VK_CHECK(vkCreateGraphicsPipelines(logical_device, pipelineCache, 1, &createInfo, 0, &pipelines[i]));
		}
		return;
	}
	std::cout<<"Info: Run out of pipelines. \n";
	pipelineCount = 0;
	return;
}

void BindPipeline(VkPipeline pipeline)
{
	if(current_pipeline != pipeline)
	{
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		current_pipeline = pipeline;
	}
}

} //namespace render
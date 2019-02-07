#include "render.h"
#include "control.h"

/* {
GVAR: logical_device -> startup.cpp	
GVAR: command_buffer -> control.h
GVAR: image_format -> swapchain.cpp
} */

//{
VkRenderPass renderPasses[100] = {};
VkFramebuffer framebuffers[100] = {};
VkImageView imageViews[100] = {};
//}

static int renderPassCount = 0;
static int framebufferCount = 0;
static int imageViewCount = 0;

void CreateRenderPass(VkFormat depthFormat, bool late)
{
	VkRenderPass renderPass = 0;
	
	VkAttachmentDescription attachments[2] = {};
	attachments[0].format = image_format; 
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = late ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[1].format = depthFormat;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = late ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = late ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachment = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depthAttachment = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpass = {};
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachment;
	subpass.pDepthStencilAttachment = &depthAttachment;

	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	//createInfo.flags = ;
	createInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
	createInfo.pAttachments = attachments;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;
	
	VK_CHECK(vkCreateRenderPass(logical_device, &createInfo, 0, &renderPass));
	
	if(renderPassCount == ARRAYSIZE(renderPasses))
	{
		std::cout<<"Info: Run out of render passes. \n";
		renderPassCount = 0; 
		vkDestroyRenderPass(logical_device, renderPasses[renderPassCount], nullptr);
	}
	renderPasses[renderPassCount] = renderPass;
	renderPassCount++;
}

void DestroyRenderPass(int render_index)
{
	vkDestroyRenderPass(logical_device, renderPasses[render_index], nullptr);
}

void CreateFramebuffer(VkImageView colorView, VkImageView depthView, int render_index, uint32_t width, uint32_t height)
{
	VkFramebuffer framebuffer = 0;
	
	VkImageView attachments[] = { colorView, depthView };

	VkFramebufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	createInfo.renderPass = renderPasses[render_index];
	createInfo.attachmentCount = ARRAYSIZE(attachments);
	createInfo.pAttachments = attachments;
	createInfo.width = width;
	createInfo.height = height;
	createInfo.layers = 1;
	
	VK_CHECK(vkCreateFramebuffer(logical_device, &createInfo, 0, &framebuffer));
	
	if(framebufferCount == ARRAYSIZE(framebuffers))
	{
		std::cout<<"Info: Run out of frame buffers. \n";
		framebufferCount = 0; 
		vkDestroyFramebuffer(logical_device, framebuffers[framebufferCount], nullptr);
	}
	framebuffers[framebufferCount] = framebuffer;
	framebufferCount++;	
}

void CreateImageView(VkImage image, VkFormat format, uint32_t mipLevel, uint32_t levelCount)
{
	VkImageAspectFlags aspectMask = (format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = aspectMask;
	createInfo.subresourceRange.baseMipLevel = mipLevel;
	createInfo.subresourceRange.levelCount = levelCount;
	createInfo.subresourceRange.layerCount = 1;

	VkImageView view = 0;
	VK_CHECK(vkCreateImageView(logical_device, &createInfo, 0, &view));
	
	if(imageViewCount == ARRAYSIZE(framebuffers))
	{
		std::cout<<"Info: Run out of imageViews. \n";
		imageViewCount = 0; 
	}
	imageViews[imageViewCount] = view;
	imageViewCount++;		
}

void DestroyFramebuffer(int buffer_index)
{
	vkDestroyFramebuffer(logical_device, framebuffers[buffer_index], nullptr);
}

void StartRenderPass(VkRect2D render_area, VkClearValue *clear_values, VkSubpassContents subpass_contents, int render_index, int buffer_index)
{
	VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
	render_pass_begin_info.renderPass = renderPasses[render_index];
    render_pass_begin_info.framebuffer = framebuffers[buffer_index];
    render_pass_begin_info.renderArea = render_area;
    render_pass_begin_info.clearValueCount = ARRAYSIZE(clear_values);
    render_pass_begin_info.pClearValues = clear_values;
	
	vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, subpass_contents);
}
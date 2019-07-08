#include "render.h"
#include "control.h"
#include "window.h"

/* {
GVAR: logical_device -> startup.cpp
GVAR: command_buffer -> control.h
GVAR: ubo_dsl -> control.cpp
GVAR: tex_dsl -> control.cpp
GVAR: image_format -> swapchain.cpp
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
	createInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	createInfo.subresourceRange.aspectMask = aspectMask;
	createInfo.subresourceRange.baseMipLevel = mipLevel;
	createInfo.subresourceRange.levelCount = levelCount;
	createInfo.subresourceRange.layerCount = 1;

	if(imageViewCount < ARRAYSIZE(imageViews))
	{
		for(int i = 0; i<count; i++)
		{
			createInfo.image = image[i];
			uint32_t inc = (!imageViewCount) ? imageViewCount + i : imageViewCount + i + 1;
			VK_CHECK(vkCreateImageView(logical_device, &createInfo, 0, &imageViews[inc]));
		}
		imageViewCount += count;
		return;
	}
	std::cout<<"Info: Run out of imageViews. \n";
	imageViewCount = 0;
	return;
}

VkImage Create2DImage(VkImageUsageFlags usage, int w, int h)
{
	VkImage img;
	VkImageCreateInfo image_create_info;
	memset(&image_create_info, 0, sizeof(image_create_info));
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
	image_create_info.extent.width = w;
	image_create_info.extent.height = h;
	image_create_info.extent.depth = 1;
	image_create_info.mipLevels = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.usage = usage;
	image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VK_CHECK(vkCreateImage(logical_device, &image_create_info, nullptr, &img));
	return img;
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

void CreatePipelineLayout()
{

	VkDescriptorSetLayout basic_descriptor_set_layouts[2] = {ubo_dsl, tex_dsl};

	/* 	VkPushConstantRange push_constant_range;
		memset(&push_constant_range, 0, sizeof(push_constant_range));
		push_constant_range.offset = 0;
		push_constant_range.size = 21 * sizeof(float);
		push_constant_range.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS; */

	VkPipelineLayoutCreateInfo createInfo;
	memset(&createInfo, 0, sizeof(createInfo));
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.setLayoutCount = 2;
	createInfo.pSetLayouts = basic_descriptor_set_layouts;
	//createInfo.pushConstantRangeCount = 1;
	//createInfo.pPushConstantRanges = &push_constant_range;

	VK_CHECK(vkCreatePipelineLayout(logical_device, &createInfo, 0, &pipeline_layout));
}

VkPipelineVertexInputStateCreateInfo* BasicTrianglePipe()
{
        zone::stack_alloc(100000, 1);

	VkVertexInputBindingDescription* bindingDescription = new(stack_mem) VkVertexInputBindingDescription[0];
	bindingDescription[0].binding = 0;
	bindingDescription[0].stride = sizeof(Vertex_);
	bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription* attributeDescriptions = new(bindingDescription+sizeof(bindingDescription)) VkVertexInputAttributeDescription[3];
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex_, pos);
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex_, color);
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex_, tex_coord);

	VkPipelineVertexInputStateCreateInfo* vertexInput = new(&attributeDescriptions[0] + sizeof(attributeDescriptions)) VkPipelineVertexInputStateCreateInfo[0];
	vertexInput[0].sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput[0].vertexBindingDescriptionCount = 1;
	vertexInput[0].vertexAttributeDescriptionCount = 3;
	vertexInput[0].pVertexBindingDescriptions = bindingDescription;
	vertexInput[0].pVertexAttributeDescriptions = &attributeDescriptions[0];

	return vertexInput;
}

void CreateGraphicsPipelines
(uint32_t count, VkPipelineCache pipelineCache, VkPipelineVertexInputStateCreateInfo* (*vertexInput)(), int render_index, VkShaderModule vs, VkShaderModule fs)
{
	VkPipelineShaderStageCreateInfo stages[2] = {};
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = vs;
	stages[0].pName = "main";
	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = fs;
	stages[1].pName = "main";


	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = ARRAYSIZE(stages);
	createInfo.pStages = stages;
	createInfo.pVertexInputState = vertexInput();

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
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_NONE;
	rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.depthBiasEnable = VK_FALSE;
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

	uint32_t barrier = pipelineCount+count;
	if(barrier < ARRAYSIZE(pipelines))
	{
		for(; pipelineCount<barrier; ++pipelineCount)
		{
			VK_CHECK(vkCreateGraphicsPipelines(logical_device, pipelineCache, 1, &createInfo, 0, &pipelines[pipelineCount]));
		}
		return;
	}
	std::cout<<"Info: Run out of pipelines. \n";
	pipelineCount = 0;
	return;
}

} //namespace render
 

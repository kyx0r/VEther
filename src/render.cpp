#include "render.h"
#include "control.h"
#include "window.h"
#include "flog.h"

/* {
GVAR: logical_device -> startup.cpp
GVAR: allocators -> startup.cpp
GVAR: command_buffer -> control.h
GVAR: ubo_dsl -> control.cpp
GVAR: tex_dsl -> control.cpp
GVAR: image_format -> swapchain.cpp
GVAR: handle_array_of_swapchain_images -> swapchain.cpp
GVAR: number_of_swapchain_images -> swapchain.cpp
} */

//{
VkRenderPass renderPasses[10] = {};
VkFramebuffer framebuffers[10] = {};
VkImageView imageViews[100] = {};
VkPipeline pipelines[20] = {};
uint32_t renderPassCount = 0;
uint32_t framebufferCount = 0;
uint32_t imageViewCount = 0;
uint32_t pipelineCount = 0;
VkPipelineLayout pipeline_layout = 0;
VkPipeline current_pipeline = 0;
//}

static VkImage depth_buffer = VK_NULL_HANDLE;
static VkDeviceMemory depth_buffer_memory = VK_NULL_HANDLE;

namespace render
{

void CreateRenderPass(VkFormat depthFormat, bool late)
{
	VkAttachmentDescription attachments[2] = {};
	attachments[0].format = image_format;
	attachments[0].flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = late ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	attachments[1].flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].format = depthFormat;
	attachments[1].loadOp = late ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = late ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	VkAttachmentReference colorAttachment;
	colorAttachment.attachment = 0;
	colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachment;
	depthAttachment.attachment = 1;
	depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// If srcSubpass is equal to dstSubpass then the VkSubpassDependency describes a subpass self-dependency, and only constrains the pipeline barriers allowed within a subpass instance.
	// Otherwise, when a render pass instance which includes a subpass dependency is submitted to a queue, it defines a memory dependency between the subpasses identified by srcSubpass and dstSubpass.

	// If srcSubpass is equal to VK_SUBPASS_EXTERNAL, the first synchronization scope includes commands that occur earlier in submission order than the vkCmdBeginRenderPass used to begin the render pass instance.
	// Otherwise, the first set of commands includes all commands submitted as part of the subpass instance identified by srcSubpass and any load, store or multisample resolve operations on attachments used in srcSubpass.
	// In either case, the first synchronization scope is limited to operations on the pipeline stages determined by the source stage mask specified by srcStageMask.

	// If dstSubpass is equal to VK_SUBPASS_EXTERNAL, the second synchronization scope includes commands that occur later in submission order than the vkCmdEndRenderPass used to end the render pass instance.
	// Otherwise, the second set of commands includes all commands submitted as part of the subpass instance identified by dstSubpass and any load, store or multisample resolve operations on attachments used in dstSubpass.
	// In either case, the second synchronization scope is limited to operations on the pipeline stages determined by the destination stage mask specified by dstStageMask.

	// The first access scope is limited to access in the pipeline stages determined by the source stage mask specified by srcStageMask. It is also limited to access types in the source access mask specified by srcAccessMask.

	// The second access scope is limited to access in the pipeline stages determined by the destination stage mask specified by dstStageMask. It is also limited to access types in the destination access mask specified by dstAccessMask.

	// The availability and visibility operations defined by a subpass dependency affect the execution of image layout transitions within the render pass.

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkSubpassDescription subpass[1] = {};
	subpass[0].flags = 0;
	subpass[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass[0].colorAttachmentCount = 1;
	subpass[0].pColorAttachments = &colorAttachment;
	subpass[0].pDepthStencilAttachment = &depthAttachment;

	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 2;
	createInfo.pAttachments = attachments;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass[0];
	//createInfo.dependencyCount = 1;
	//createInfo.pDependencies = &dependency;

	VK_CHECK(vkCreateRenderPass(logical_device, &createInfo, 0, &renderPasses[renderPassCount++]));
	return;
}

void CreateFramebuffer(VkImageView *colorView, VkImageView *depthView, int render_index, uint32_t width, uint32_t height)
{
	VkFramebufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	createInfo.renderPass = renderPasses[render_index];
	createInfo.width = width;
	createInfo.height = height;
	createInfo.layers = 1;

	VkImageView attachments[] = { *colorView, *depthView };
	createInfo.attachmentCount = ARRAYSIZE(attachments);
	createInfo.pAttachments = attachments;
	VK_CHECK(vkCreateFramebuffer(logical_device, &createInfo, 0, &framebuffers[framebufferCount++]));
	return;
}

VkImage Create2DImage(VkFormat format, VkImageUsageFlags usage, int w, int h)
{
	VkImage img;
	VkImageCreateInfo image_create_info;
	memset(&image_create_info, 0, sizeof(image_create_info));
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.format = format;
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

void DestroyDepthBuffer()
{
	vkDestroyImage(logical_device, depth_buffer, nullptr);
	vkFreeMemory(logical_device, depth_buffer_memory, nullptr);
}

void CreateDepthBuffer()
{
	trace("Creating depth buffer\n");

	VkResult err;
	if(depth_buffer)
	{
		DestroyDepthBuffer();
		vkDestroyImageView(logical_device, imageViews[number_of_swapchain_images], nullptr);
	}

	//todo: check if this is supported before attempting.
	depth_buffer = Create2DImage(VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, window_width, window_height);

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(logical_device, depth_buffer, &memory_requirements);

	VkMemoryDedicatedAllocateInfoKHR dedicated_allocation_info;
	memset(&dedicated_allocation_info, 0, sizeof(dedicated_allocation_info));
	dedicated_allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR;
	dedicated_allocation_info.image = depth_buffer;

	VkMemoryAllocateInfo memory_allocate_info;
	memset(&memory_allocate_info, 0, sizeof(memory_allocate_info));
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = control::MemoryTypeFromProperties(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);

	err = vkAllocateMemory(logical_device, &memory_allocate_info, nullptr, &depth_buffer_memory);
	if (err != VK_SUCCESS)
		error("vkAllocateMemory failed\n");

	err = vkBindImageMemory(logical_device, depth_buffer, depth_buffer_memory, 0);
	if (err != VK_SUCCESS)
		error("vkBindImageMemory failed\n");

	VkImageViewCreateInfo image_view_create_info;
	memset(&image_view_create_info, 0, sizeof(image_view_create_info));
	image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.format = VK_FORMAT_D32_SFLOAT;
	image_view_create_info.image = depth_buffer;
	image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	image_view_create_info.subresourceRange.baseMipLevel = 0;
	image_view_create_info.subresourceRange.levelCount = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount = 1;
	image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.flags = 0;

	err = vkCreateImageView(logical_device, &image_view_create_info, nullptr, &imageViews[imageViewCount++]);
	if (err != VK_SUCCESS)
		error("vkCreateImageView failed");

	// VkImageMemoryBarrier image_memory_barrier;
	// memset(&image_memory_barrier, 0, sizeof(image_memory_barrier));
	// image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	// image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	// image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	// image_memory_barrier.image = depth_buffer;
	// image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	// image_memory_barrier.subresourceRange.baseMipLevel = 0;
	// image_memory_barrier.subresourceRange.levelCount = 1;
	// image_memory_barrier.subresourceRange.baseArrayLayer = 0;
	// image_memory_barrier.subresourceRange.layerCount = 1;

	// image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	// image_memory_barrier.srcAccessMask = 0;
	// image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	// vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
}


void CreateSwapchainImageViews()
{
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = image_format;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.layerCount = 1;

	for(uint16_t i = 0; i<number_of_swapchain_images; i++)
	{
		createInfo.image = handle_array_of_swapchain_images[i];
		VK_CHECK(vkCreateImageView(logical_device, &createInfo, 0, &imageViews[imageViewCount + i]));
	}
	imageViewCount += number_of_swapchain_images;
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
	render_pass_begin_info.clearValueCount = 2;
	render_pass_begin_info.pClearValues = clear_values;

	vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, subpass_contents);
}

void CreatePipelineLayout()
{

	VkDescriptorSetLayout basic_descriptor_set_layouts[2] = {ubo_dsl, tex_dsl};

	VkPushConstantRange push_constant_range;
	memset(&push_constant_range, 0, sizeof(push_constant_range));
	push_constant_range.offset = 0;
	push_constant_range.size = 21 * sizeof(float); //limit is 256 bytes
	push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineLayoutCreateInfo createInfo;
	memset(&createInfo, 0, sizeof(createInfo));
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.setLayoutCount = 2;
	createInfo.pSetLayouts = basic_descriptor_set_layouts;
	createInfo.pushConstantRangeCount = 1;
	createInfo.pPushConstantRanges = &push_constant_range;

	VK_CHECK(vkCreatePipelineLayout(logical_device, &createInfo, allocators, &pipeline_layout));
}

void Viewport(float x, float y, float width, float height, float min_depth, float max_depth)
{
	VkViewport viewport;
	viewport.x = x;
	viewport.y = window_height - (y + height);
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = min_depth;
	viewport.maxDepth = max_depth;

	vkCmdSetViewport(command_buffer, 0, 1, &viewport);
}

VkPipelineVertexInputStateCreateInfo* BasicTrianglePipe()
{
	zone::stack_alloc(100000);

	VkVertexInputBindingDescription* bindingDescription = new(stack_mem) VkVertexInputBindingDescription[0];
	bindingDescription[0].binding = 0;
	bindingDescription[0].stride = sizeof(Vertex_);
	bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription* attributeDescriptions = new(bindingDescription+sizeof(bindingDescription)) VkVertexInputAttributeDescription[3];
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex_, pos);
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex_, tex_coord);
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex_, color);

	VkPipelineVertexInputStateCreateInfo* vertexInput = new(&attributeDescriptions[0] + sizeof(attributeDescriptions)) VkPipelineVertexInputStateCreateInfo[0];
	vertexInput[0].sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput[0].pNext = nullptr;
	vertexInput[0].flags = 0;
	vertexInput[0].vertexBindingDescriptionCount = 1;
	vertexInput[0].vertexAttributeDescriptionCount = 3;
	vertexInput[0].pVertexBindingDescriptions = bindingDescription;
	vertexInput[0].pVertexAttributeDescriptions = &attributeDescriptions[0];

	return vertexInput;
}

VkPipelineVertexInputStateCreateInfo* ScreenPipe()
{
	zone::stack_alloc(100000);
	VkVertexInputBindingDescription* bindingDescription = new(stack_mem) VkVertexInputBindingDescription[0];
	bindingDescription[0].binding = 0;
	bindingDescription[0].stride = sizeof(Uivertex);
	bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription* attributeDescriptions = new(bindingDescription+sizeof(bindingDescription)) VkVertexInputAttributeDescription[3];
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Uivertex, pos);
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Uivertex, tex_coord);
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32_UINT;
	attributeDescriptions[2].offset = offsetof(Uivertex, color);

	VkPipelineVertexInputStateCreateInfo* vertexInput = new(&attributeDescriptions[0] + sizeof(attributeDescriptions)) VkPipelineVertexInputStateCreateInfo[0];
	vertexInput[0].sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput[0].vertexBindingDescriptionCount = 1;
	vertexInput[0].vertexAttributeDescriptionCount = 3;
	vertexInput[0].pVertexBindingDescriptions = bindingDescription;
	vertexInput[0].pVertexAttributeDescriptions = &attributeDescriptions[0];

	return vertexInput;
}

void CreateGraphicsPipeline
(VkPipelineCache pipelineCache, VkPipelineVertexInputStateCreateInfo* (*vertexInput)(), int render_index, VkShaderModule vs, VkShaderModule fs)
{
	ASSERT(vs, "Failed to load Vertex Shader.");
	ASSERT(fs, "Failed to load Fragment Shader.");

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

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &colorBlendAttachment;
	createInfo.pColorBlendState = &colorBlendState;

	if(vertexInput == ScreenPipe)
	{
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		colorBlendState.logicOpEnable = VK_FALSE;
		colorBlendState.logicOp = VK_LOGIC_OP_COPY;
		colorBlendState.attachmentCount = 1;
		colorBlendState.blendConstants[0] = 1.0f;
		colorBlendState.blendConstants[1] = 1.0f;
		colorBlendState.blendConstants[2] = 1.0f;
		colorBlendState.blendConstants[3] = 1.0f;
	}
	else
	{
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.stencilTestEnable = VK_FALSE;
	}

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = ARRAYSIZE(dynamicStates);
	dynamicState.pDynamicStates = dynamicStates;
	createInfo.pDynamicState = &dynamicState;

	createInfo.layout = pipeline_layout;
	createInfo.renderPass = renderPasses[render_index];

	VK_CHECK(vkCreateGraphicsPipelines(logical_device, pipelineCache, 1, &createInfo, 0, &pipelines[pipelineCount++]));
	return;
}

void CreateTessGraphicsPipeline
(VkPipelineCache pipelineCache, VkPipelineVertexInputStateCreateInfo* (*vertexInput)(),
 int render_index, VkShaderModule vs, VkShaderModule fs,  VkShaderModule cs, VkShaderModule es)
{
	ASSERT(vs, "Failed to load Vertex Shader.");
	ASSERT(fs, "Failed to load Fragment Shader.");
	ASSERT(cs, "Failed to load Tesselation Control Shader.");
	ASSERT(es, "Failed to load Tesselation Evaluation Shader.");

	VkPipelineShaderStageCreateInfo stages[4] = {};
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = vs;
	stages[0].pName = "main";
	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	stages[1].module = cs;
	stages[1].pName = "main";
	stages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[2].stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	stages[2].module = es;
	stages[2].pName = "main";
	stages[3].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[3].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[3].module = fs;
	stages[3].pName = "main";

	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = ARRAYSIZE(stages);
	createInfo.pStages = stages;
	createInfo.pVertexInputState = vertexInput();

	VkPipelineTessellationStateCreateInfo tsci;
	tsci.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	tsci.pNext = nullptr;
	tsci.flags = 0;
	tsci.patchControlPoints = 3;
	createInfo.pTessellationState = &tsci;
	
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
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

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &colorBlendAttachment;
	createInfo.pColorBlendState = &colorBlendState;

	if(vertexInput == ScreenPipe)
	{
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		colorBlendState.logicOpEnable = VK_FALSE;
		colorBlendState.logicOp = VK_LOGIC_OP_COPY;
		colorBlendState.attachmentCount = 1;
		colorBlendState.blendConstants[0] = 1.0f;
		colorBlendState.blendConstants[1] = 1.0f;
		colorBlendState.blendConstants[2] = 1.0f;
		colorBlendState.blendConstants[3] = 1.0f;
	}
	else
	{
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.stencilTestEnable = VK_FALSE;
	}

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = ARRAYSIZE(dynamicStates);
	dynamicState.pDynamicStates = dynamicStates;
	createInfo.pDynamicState = &dynamicState;

	createInfo.layout = pipeline_layout;
	createInfo.renderPass = renderPasses[render_index];

	VK_CHECK(vkCreateGraphicsPipelines(logical_device, pipelineCache, 1, &createInfo, 0, &pipelines[pipelineCount++]));
	return;

}

} //namespace render


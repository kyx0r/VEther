#include "control.h"
#include "window.h"
#include "zone.h"

/* {
GVAR: logical_device -> startup.cpp
GVAR: ComputeQueue -> window.cpp
} */

//{
VkCommandBuffer command_buffer;
VkPhysicalDeviceMemoryProperties memory_properties;
VkDescriptorPool descriptor_pool;
VkDescriptorSetLayout ubo_dsl;
VkDescriptorSetLayout tex_dsl;
vram_heap texmgr_heaps[TEXTURE_MAX_HEAPS];
dynbuffer_t dyn_index_buffers[NUM_DYNAMIC_BUFFERS];
dynbuffer_t dyn_vertex_buffers[NUM_DYNAMIC_BUFFERS];
dynbuffer_t dyn_uniform_buffers[NUM_DYNAMIC_BUFFERS];
stagingbuffer_t	staging_buffers[NUM_STAGING_BUFFERS];
uint8_t current_cmd_buffer_index = 0;
int current_dyn_buffer_index = 0; //influences vertex & uniform buffers.
int current_staging_buffer = 1;
int current_vheap_index  = 0;
//}

static VkCommandPool command_pool;
static VkCommandBuffer *command_buffers = nullptr;
static VkDeviceMemory dyn_index_buffer_memory;
static VkDeviceMemory dyn_vertex_buffer_memory;
static VkDeviceMemory dyn_uniform_buffer_memory;
static VkDeviceMemory	staging_memory;
static VkDescriptorSet	ubo_descriptor_sets[2];

namespace control
{

bool CreateCommandPool(VkCommandPoolCreateFlags parameters, uint32_t queue_family)
{
	VkCommandPoolCreateInfo command_pool_create_info =
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,   // VkStructureType              sType
		nullptr,                                      // const void                 * pNext
		parameters,                                   // VkCommandPoolCreateFlags     flags
		queue_family                                  // uint32_t                     queueFamilyIndex
	};

	VkResult result = vkCreateCommandPool(logical_device, &command_pool_create_info, nullptr, &command_pool);
	if(result != VK_SUCCESS)
	{
		std::cout << "Could not create command pool." << std::endl;
		return false;
	}
	return true;
}

bool ResetCommandPool(bool release_resources)
{
	//VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT -> let the driver handle the pool automatically.
	//VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT -> this flag is not required at pool creation if you use this function.
	VkResult result = vkResetCommandPool(logical_device, command_pool, release_resources ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0);
	if(result != VK_SUCCESS)
	{
		std::cout << "Error occurred during command pool reset." << std::endl;
		return false;
	}
	return true;
}

bool AllocateCommandBuffers(VkCommandBufferLevel level, uint32_t count)
{
	VkCommandBufferAllocateInfo command_buffer_allocate_info =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,   // VkStructureType          sType
		nullptr,                                          // const void             * pNext
		command_pool,                                     // VkCommandPool            commandPool
		level,                                            // VkCommandBufferLevel     level
		count                                             // uint32_t                 commandBufferCount
	};

	char* mem = (char*) zone::Z_Malloc(sizeof(VkCommandBuffer) * count);
	command_buffers = new(mem) VkCommandBuffer[count];

	VkResult result = vkAllocateCommandBuffers(logical_device, &command_buffer_allocate_info, &command_buffers[0]);
	if(result != VK_SUCCESS)
	{
		std::cout << "Could not allocate command buffers. \n";
		return false;
	}
	command_buffer = command_buffers[0];
	return true;
}

void SetCommandBuffer(uint8_t i)
{
	current_cmd_buffer_index = i;
	command_buffer = command_buffers[i];
}

bool CreateSemaphore(VkSemaphore &semaphore)
{
	VkSemaphoreCreateInfo semaphore_create_info =
	{
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,    // VkStructureType            sType
		nullptr,                                    // const void               * pNext
		0                                           // VkSemaphoreCreateFlags     flags
	};

	VkResult result = vkCreateSemaphore(logical_device, &semaphore_create_info, nullptr, &semaphore);
	if(result != VK_SUCCESS)
	{
		std::cout << "Could not create a semaphore." << std::endl;
		return false;
	}
	return true;
}

bool CreateFence(VkFence &fence, VkSemaphoreCreateFlags flags)
{
	VkFenceCreateInfo fenceInfo =
	{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,    	// VkStructureType            sType
		nullptr,                                    // const void               * pNext
		flags               						// VkSemaphoreCreateFlags     flags
	};

	VkResult result = vkCreateFence(logical_device, &fenceInfo, nullptr, &fence);
	if(result != VK_SUCCESS)
	{
		std::cout << "Could not create a fence." << std::endl;
		return false;
	}
	return true;
}

bool BeginCommandBufferRecordingOperation(VkCommandBufferUsageFlags usage, VkCommandBufferInheritanceInfo *secondary_command_buffer_info)
{
	VkCommandBufferBeginInfo command_buffer_begin_info =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,    // VkStructureType                        sType
		nullptr,                                        // const void                           * pNext
		usage,                                          // VkCommandBufferUsageFlags              flags
		secondary_command_buffer_info                   // const VkCommandBufferInheritanceInfo * pInheritanceInfo
	};

	VkResult result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
	if(result != VK_SUCCESS)
	{
		std::cout << "Could not begin command buffer recording operation." << std::endl;
		return false;
	}
	return true;
}

void CreateDescriptorPool()
{
	VkDescriptorPoolSize pool_sizes[2] = {};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	pool_sizes[0].descriptorCount = 32;
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount = 2048;

	VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
	descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_create_info.maxSets = 2048 + 32;
	descriptor_pool_create_info.poolSizeCount = 2;
	descriptor_pool_create_info.pPoolSizes = pool_sizes;
	descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	VK_CHECK(vkCreateDescriptorPool(logical_device, &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void CreateDescriptorSetLayouts()
{
	//ubo = uniform buffer object
	VkDescriptorSetLayoutBinding ubo = {};
	ubo.binding = 0;
	ubo.descriptorCount = 1;
	ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	ubo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	ubo.pImmutableSamplers = nullptr;

	//slb = sampler layout binding
	VkDescriptorSetLayoutBinding slb = {};
	slb.binding = 0;
	slb.descriptorCount = 1;
	slb.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	slb.pImmutableSamplers = nullptr;
	slb.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//dslci = descriptor set layout create info
	VkDescriptorSetLayoutCreateInfo dslci = {};
	dslci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	dslci.bindingCount = 1;
	dslci.pBindings = &ubo;

	VK_CHECK(vkCreateDescriptorSetLayout(logical_device, &dslci, nullptr, &ubo_dsl));
	dslci.pBindings = &slb;
	VK_CHECK(vkCreateDescriptorSetLayout(logical_device, &dslci, nullptr, &tex_dsl));
}

void BindDescriptorSet
(	VkPipelineBindPoint                         pipelineBindPoint,
    VkPipelineLayout                            layout,
    uint32_t                                    firstSet,
    uint32_t                                    descriptorSetCount,
    uint32_t                                    dynamicOffsetCount,
    const uint32_t*                             pDynamicOffsets)
{
	vkCmdBindDescriptorSets(command_buffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, &ubo_descriptor_sets[current_dyn_buffer_index], dynamicOffsetCount, pDynamicOffsets);
}

bool EndCommandBufferRecordingOperation()
{
	VkResult result = vkEndCommandBuffer(command_buffer);
	if(result != VK_SUCCESS)
	{
		std::cout << "Error occurred during command buffer recording." << std::endl;
		return false;
	}
	return true;
}

bool ResetCommandBuffer(bool release_resources)
{
	//VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT allows to have access to every buffer in command_buffers.
	VkResult result = vkResetCommandBuffer(command_buffer, release_resources ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0);
	if(result != VK_SUCCESS)
	{
		std::cout << "Error occurred during command buffer reset." << std::endl;
		return false;
	}
	return true;
}

void FreeCommandBuffers(uint32_t count)
{
	vkFreeCommandBuffers(logical_device, command_pool, count, command_buffers);
}

void DestroyCommandPool()
{
	vkDestroyCommandPool(logical_device, command_pool, nullptr);
}

int MemoryTypeFromProperties(uint32_t type_bits, VkFlags requirements_mask, VkFlags preferred_mask)
{
	uint32_t current_type_bits = type_bits;
	uint32_t i;

	for (i = 0; i < VK_MAX_MEMORY_TYPES; i++)
	{
		if ((current_type_bits & 1) == 1)
		{
			if ((memory_properties.memoryTypes[i].propertyFlags & (requirements_mask | preferred_mask)) == (requirements_mask | preferred_mask))
				return i;
		}
		current_type_bits >>= 1;
	}

	current_type_bits = type_bits;
	for (i = 0; i < VK_MAX_MEMORY_TYPES; i++)
	{
		if ((current_type_bits & 1) == 1)
		{
			if ((memory_properties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask)
				return i;
		}
		current_type_bits >>= 1;
	}

	printf("Could not find memory type \n");
	return 0;
}

/*
==============================================================================

					GENARAL VRAM ALLOCATIONS

==============================================================================
*/

void VramHeapAllocate(VkDeviceSize size, uint32_t memory_type_index)
{
	vram_heap* heap = &texmgr_heaps[current_vheap_index];

	VkMemoryAllocateInfo memory_allocate_info;
	memset(&memory_allocate_info, 0, sizeof(memory_allocate_info));
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = size;
	memory_allocate_info.memoryTypeIndex = memory_type_index;

	VkResult err = vkAllocateMemory(logical_device, &memory_allocate_info, nullptr, &heap->memory);
	if (err != VK_SUCCESS)
		printf("vkAllocateMemory failed");

	heap->offset = 0;
	heap->size = size;
	heap->prev = nullptr;
	heap->next = nullptr;
	heap->free = true;
}

vram_heap* VramHeapDigress(VkDeviceSize size, VkDeviceSize alignment, VkDeviceSize* aligned_offset)
{
	vram_heap* current_node = &texmgr_heaps[current_vheap_index];
	vram_heap* best_fit_node = nullptr;
	VkDeviceSize best_fit_size = UINT64_MAX;

	for ( ; ; current_node = current_node->next)
	{
		if (!current_node->free)
			continue;

		const VkDeviceSize align_mod = current_node->offset % alignment;
		VkDeviceSize align_padding = (align_mod == 0) ? 0 : (alignment - align_mod);
		VkDeviceSize aligned_size = size + align_padding;

		if (current_node->size == aligned_size)
		{
			current_node->free = false;
			*aligned_offset = current_node->offset + align_padding;
			return current_node;
		}
		else if ((current_node->size > aligned_size) && (current_node->size < best_fit_size))
		{
			best_fit_size = current_node->size;
			best_fit_node = current_node;
			vram_heap* new_node = &texmgr_heaps[++current_vheap_index];
			*new_node = *best_fit_node;
			new_node->prev = best_fit_node->prev;
			new_node->next = best_fit_node;
			new_node->memory = best_fit_node->memory;
			if(best_fit_node->prev)
				best_fit_node->prev->next = new_node;
			best_fit_node->prev = new_node;
			new_node->free = false;

			new_node->size = aligned_size;
			best_fit_node->size -= aligned_size;
			best_fit_node->offset += aligned_size;

			*aligned_offset = new_node->offset + align_padding;
			return new_node;
		}
	}

	*aligned_offset = 0;
	return nullptr;
}

void DestroyVramHeaps()
{
  for(int i = 0; i < TEXTURE_MAX_HEAPS; ++i)
  {
      vram_heap* heap = &texmgr_heaps[i];
      if(heap->offset == 0)
      {
	vkFreeMemory(logical_device, heap->memory, nullptr);
      }
	
  }
	
}

/*
===============

IndexBuffersAllocate

===============
*/

unsigned char* IndexBufferDigress(int size, VkBuffer* buffer, VkDeviceSize* buffer_offset)
{
	// Align to 4 bytes because we allocate both uint16 and uint32
	// index buffers and alignment must match index size
	const int align_mod = size % 4;
	const int aligned_size = ((size % 4) == 0) ? size : (size + 4 - align_mod);

	dynbuffer_t *dyn_ib = &dyn_index_buffers[current_dyn_buffer_index];

	if ((dyn_ib->current_offset + aligned_size) > (DYNAMIC_INDEX_BUFFER_SIZE_KB * 1024))
	{
		printf("Out of dynamic index buffer space, increase DYNAMIC_INDEX_BUFFER_SIZE_KB \n");
		dyn_ib->current_offset = 0;
	}

	*buffer = dyn_ib->buffer;
	*buffer_offset = dyn_ib->current_offset;

	unsigned char *data = dyn_ib->data + dyn_ib->current_offset;
	dyn_ib->current_offset += aligned_size;

	return data;
}

void IndexBuffersAllocate()
{
	int i;
	printf("Initializing dynamic index buffers\n");

	VkResult err;
	VkBufferCreateInfo buffer_create_info;
	memset(&buffer_create_info, 0, sizeof(buffer_create_info));
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = DYNAMIC_INDEX_BUFFER_SIZE_KB * 1024;
	buffer_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	for (i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
	{
		dyn_index_buffers[i].current_offset = 0;

		err = vkCreateBuffer(logical_device, &buffer_create_info, nullptr, &dyn_index_buffers[i].buffer);
		if (err != VK_SUCCESS)
			printf("vkCreateBuffer failed \n");
	}

	VkMemoryRequirements memory_requirements;
	for (i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
	{
		vkGetBufferMemoryRequirements(logical_device, dyn_index_buffers[i].buffer, &memory_requirements);
	}
	const int align_mod = memory_requirements.size % memory_requirements.alignment;
	const int aligned_size = ((memory_requirements.size % memory_requirements.alignment) == 0)
	                         ? memory_requirements.size
	                         : (memory_requirements.size + memory_requirements.alignment - align_mod);

	VkMemoryAllocateInfo memory_allocate_info;
	memset(&memory_allocate_info, 0, sizeof(memory_allocate_info));
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = NUM_DYNAMIC_BUFFERS * aligned_size;
	memory_allocate_info.memoryTypeIndex = MemoryTypeFromProperties(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

	err = vkAllocateMemory(logical_device, &memory_allocate_info, nullptr, &dyn_index_buffer_memory);
	if (err != VK_SUCCESS)
		printf("vkAllocateMemory failed");

	for (i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
	{
		err = vkBindBufferMemory(logical_device, dyn_index_buffers[i].buffer, dyn_index_buffer_memory, i * aligned_size);
		if (err != VK_SUCCESS)
			printf("vkBindBufferMemory failed");
	}

	void* data;
	err = vkMapMemory(logical_device, dyn_index_buffer_memory, 0, NUM_DYNAMIC_BUFFERS * aligned_size, 0, &data);
	if (err != VK_SUCCESS)
		printf("vkMapMemory failed");

	for (i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
		dyn_index_buffers[i].data = (unsigned char *)data + (i * aligned_size);
}

void DestroyIndexBuffers()
{
  for (int i = 0; i < NUM_STAGING_BUFFERS; ++i)
  {
    vkDestroyBuffer(logical_device, dyn_index_buffers[i].buffer, nullptr);
  }

  vkFreeMemory(logical_device, dyn_index_buffer_memory, nullptr);
}

/*
==============================================================================

					UNIFORM BUFFERS


==============================================================================
*/

unsigned char* UniformBufferDigress(int size, VkBuffer* buffer, uint32_t* buffer_offset, VkDescriptorSet* descriptor_set)
{
	if (size > MAX_UNIFORM_ALLOC)
		printf("Increase MAX_UNIFORM_ALLOC");

	const int align_mod = size % 256;
	const int aligned_size = ((size % 256) == 0) ? size : (size + 256 - align_mod);

	dynbuffer_t *dyn_ub = &dyn_uniform_buffers[current_dyn_buffer_index];

	if ((dyn_ub->current_offset + MAX_UNIFORM_ALLOC) > (DYNAMIC_UNIFORM_BUFFER_SIZE_KB * 1024))
	{
		printf("Out of dynamic uniform buffer space, increase DYNAMIC_UNIFORM_BUFFER_SIZE_KB \n");
		dyn_ub->current_offset = 0;
	}
	*buffer = dyn_ub->buffer;
	*buffer_offset = dyn_ub->current_offset;

	unsigned char *data = dyn_ub->data + dyn_ub->current_offset;
	dyn_ub->current_offset += aligned_size;

	*descriptor_set = ubo_descriptor_sets[current_dyn_buffer_index];
	return data;
}

void UniformBuffersAllocate()
{
	int i;

	printf("Initializing dynamic uniform buffers\n");

	VkResult err;

	VkBufferCreateInfo buffer_create_info;
	memset(&buffer_create_info, 0, sizeof(buffer_create_info));
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = DYNAMIC_UNIFORM_BUFFER_SIZE_KB * 1024;
	buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	for (i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
	{
		dyn_uniform_buffers[i].current_offset = 0;

		err = vkCreateBuffer(logical_device, &buffer_create_info, nullptr, &dyn_uniform_buffers[i].buffer);
		if (err != VK_SUCCESS)
			printf("vkCreateBuffer failed \n");
	}

	VkMemoryRequirements memory_requirements;
	for (i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
	{
		vkGetBufferMemoryRequirements(logical_device, dyn_uniform_buffers[i].buffer, &memory_requirements);
	}

	const int align_mod = memory_requirements.size % memory_requirements.alignment;
	const int aligned_size = ((memory_requirements.size % memory_requirements.alignment) == 0)
	                         ? memory_requirements.size
	                         : (memory_requirements.size + memory_requirements.alignment - align_mod);

	VkMemoryAllocateInfo memory_allocate_info;
	memset(&memory_allocate_info, 0, sizeof(memory_allocate_info));
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = NUM_DYNAMIC_BUFFERS * aligned_size;
	memory_allocate_info.memoryTypeIndex = MemoryTypeFromProperties(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

	//num_vulkan_dynbuf_allocations += 1;
	err = vkAllocateMemory(logical_device, &memory_allocate_info, nullptr, &dyn_uniform_buffer_memory);
	if (err != VK_SUCCESS)
		printf("vkAllocateMemory failed \n");

	for (i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
	{
		err = vkBindBufferMemory(logical_device, dyn_uniform_buffers[i].buffer, dyn_uniform_buffer_memory, i * aligned_size);
		if (err != VK_SUCCESS)
			printf("vkBindBufferMemory failed \n");
	}

	void* data;
	err = vkMapMemory(logical_device, dyn_uniform_buffer_memory, 0, NUM_DYNAMIC_BUFFERS * aligned_size, 0, &data);
	if (err != VK_SUCCESS)
		printf("vkMapMemory failed \n");

	for (i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
		dyn_uniform_buffers[i].data = (unsigned char *)data + (i * aligned_size);

	CreateDescriptorPool();
	CreateDescriptorSetLayouts();

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
	memset(&descriptor_set_allocate_info, 0, sizeof(descriptor_set_allocate_info));
	descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.descriptorPool = descriptor_pool;
	descriptor_set_allocate_info.descriptorSetCount = 1;
	descriptor_set_allocate_info.pSetLayouts = &ubo_dsl;

	vkAllocateDescriptorSets(logical_device, &descriptor_set_allocate_info, &ubo_descriptor_sets[0]);
	vkAllocateDescriptorSets(logical_device, &descriptor_set_allocate_info, &ubo_descriptor_sets[1]);

	VkDescriptorBufferInfo buffer_info;
	memset(&buffer_info, 0, sizeof(buffer_info));
	buffer_info.offset = 0;
	buffer_info.range = MAX_UNIFORM_ALLOC;

	VkWriteDescriptorSet ubo_write;
	memset(&ubo_write, 0, sizeof(ubo_write));
	ubo_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	ubo_write.dstBinding = 0;
	ubo_write.dstArrayElement = 0;
	ubo_write.descriptorCount = 1;
	ubo_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	ubo_write.pBufferInfo = &buffer_info;

	for (i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
	{
		buffer_info.buffer = dyn_uniform_buffers[i].buffer;
		ubo_write.dstSet = ubo_descriptor_sets[i];
		vkUpdateDescriptorSets(logical_device, 1, &ubo_write, 0, nullptr);
	}
}

void DestroyUniformBuffers()
{
  for (int i = 0; i < NUM_STAGING_BUFFERS; ++i)
  {
    vkDestroyBuffer(logical_device, dyn_uniform_buffers[i].buffer, nullptr);
  }
	
  vkDestroyDescriptorSetLayout(logical_device, ubo_dsl, nullptr);
  vkDestroyDescriptorSetLayout(logical_device, tex_dsl, nullptr);
  vkFreeMemory(logical_device, dyn_uniform_buffer_memory, nullptr);
  vkDestroyDescriptorPool(logical_device, descriptor_pool, nullptr);
}

/*
==============================================================================

					STAGING BUFFERS

They are used to map the vertex data to vram.
==============================================================================
*/

void ResetStagingBuffer()
{
	VkResult err;
	stagingbuffer_t* staging_buffer = &staging_buffers[current_staging_buffer];

	if (!staging_buffers->submitted)
		return;

	err = vkWaitForFences(logical_device, 1, &staging_buffer->fence, VK_TRUE, UINT64_MAX);
	if (err != VK_SUCCESS)
		printf("vkWaitForFences failed \n");

	err = vkResetFences(logical_device, 1, &staging_buffer->fence);
	if (err != VK_SUCCESS)
		printf("vkResetFences failed \n");

	staging_buffer->current_offset = 0;
	staging_buffer->submitted = false;
	command_buffer = staging_buffer->command_buffer;
	BeginCommandBufferRecordingOperation(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
	SetCommandBuffer(0);
}

void SubmitStagingBuffer()
{
	VkMemoryBarrier memory_barrier;
	memset(&memory_barrier, 0, sizeof(memory_barrier));
	memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	memory_barrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	vkCmdPipelineBarrier(staging_buffers[current_staging_buffer].command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, &memory_barrier, 0, nullptr, 0, nullptr);

	vkEndCommandBuffer(staging_buffers[current_staging_buffer].command_buffer);

	VkMappedMemoryRange range;
	memset(&range, 0, sizeof(range));
	range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range.memory = staging_memory;
	range.size = VK_WHOLE_SIZE;
	vkFlushMappedMemoryRanges(logical_device, 1, &range);

	VkSubmitInfo submit_info;
	memset(&submit_info, 0, sizeof(submit_info));
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &staging_buffers[current_staging_buffer].command_buffer;

	vkQueueSubmit(GraphicsQueue, 1, &submit_info, staging_buffers[current_staging_buffer].fence);

	staging_buffers[current_staging_buffer].submitted = true;
	current_staging_buffer = (current_staging_buffer + 1) % NUM_STAGING_BUFFERS; //still questionable decision
}

unsigned char* StagingBufferDigress(int size, int alignment)
{
	stagingbuffer_t* staging_buffer = &staging_buffers[current_staging_buffer];
	const int align_mod = staging_buffer->current_offset % alignment;
	staging_buffer->current_offset = ((staging_buffer->current_offset % alignment) == 0)
	                                 ? staging_buffer->current_offset
	                                 : (staging_buffer->current_offset + alignment - align_mod);

	unsigned char *data = staging_buffer->data + staging_buffer->current_offset;

	return data;
}

void StagingBuffersAllocate()
{
	int i;

	printf("Initializing staging buffers\n");

	VkResult err;

	VkBufferCreateInfo buffer_create_info;
	memset(&buffer_create_info, 0, sizeof(buffer_create_info));
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = DYNAMIC_VERTEX_BUFFER_SIZE_KB * 1024;
	//memory is used for transfer only.
	buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	for (i = 0; i < NUM_STAGING_BUFFERS; ++i)
	{
		staging_buffers[i].current_offset = 0;
		staging_buffers[i].submitted = false;

		err = vkCreateBuffer(logical_device, &buffer_create_info, nullptr, &staging_buffers[i].buffer);
		if (err != VK_SUCCESS)
			printf("vkCreateBuffer failed \n");
	}

	VkMemoryRequirements memory_requirements;
	for (i = 0; i < NUM_STAGING_BUFFERS; ++i)
	{
		vkGetBufferMemoryRequirements(logical_device, staging_buffers[i].buffer, &memory_requirements);
	}

	const int align_mod = memory_requirements.size % memory_requirements.alignment;
	const int aligned_size = ((memory_requirements.size % memory_requirements.alignment) == 0)
	                         ? memory_requirements.size
	                         : (memory_requirements.size + memory_requirements.alignment - align_mod);

	VkMemoryAllocateInfo memory_allocate_info;
	memset(&memory_allocate_info, 0, sizeof(memory_allocate_info));
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = NUM_STAGING_BUFFERS * aligned_size;
	memory_allocate_info.memoryTypeIndex = MemoryTypeFromProperties(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

	err = vkAllocateMemory(logical_device, &memory_allocate_info, nullptr, &staging_memory);
	if (err != VK_SUCCESS)
		printf("vkAllocateMemory failed \n");

	for (i = 0; i < NUM_STAGING_BUFFERS; ++i)
	{
		err = vkBindBufferMemory(logical_device, staging_buffers[i].buffer, staging_memory, i * aligned_size);
		if (err != VK_SUCCESS)
			printf("vkBindBufferMemory failed \n");
	}

	void *data;
	err = vkMapMemory(logical_device, staging_memory, 0, NUM_STAGING_BUFFERS * aligned_size, 0, &data);
	if (err != VK_SUCCESS)
		printf("vkMapMemory failed \n");

	for (i = 1; i < NUM_STAGING_BUFFERS; ++i)
	{
		staging_buffers[i].data = (unsigned char *)data + (i * aligned_size);
		SetCommandBuffer(i);
		CreateFence(staging_buffers[i].fence, 0);
		BeginCommandBufferRecordingOperation(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
		staging_buffers[i].command_buffer = command_buffer;
	}
	SetCommandBuffer(0);
}

void DestroyStagingBuffers()
{
	for (int i = 0; i < NUM_STAGING_BUFFERS; ++i)
	{
		vkDestroyBuffer(logical_device, staging_buffers[i].buffer, nullptr);
	        vkDestroyFence(logical_device, staging_buffers[i].fence, nullptr);
	}
	vkFreeMemory(logical_device, staging_memory, nullptr);
}

/*
==============================================================================

					DYNAMIC VERTEX BUFFERS

==============================================================================
*/

void InvalidateDynamicBuffers()
{
	VkMappedMemoryRange ranges[1];
	memset(&ranges, 0, sizeof(ranges));
	ranges[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	ranges[0].memory = dyn_vertex_buffer_memory;
	ranges[0].size = VK_WHOLE_SIZE;
	vkInvalidateMappedMemoryRanges(logical_device, 1, ranges);
}

void FlushDynamicBuffers()
{
	VkMappedMemoryRange ranges[1];
	memset(&ranges, 0, sizeof(ranges));
	ranges[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	ranges[0].memory = dyn_vertex_buffer_memory;
	ranges[0].size = VK_WHOLE_SIZE;
	vkFlushMappedMemoryRanges(logical_device, 1, ranges);
}

void DestroyDynBuffers()
{
	for (int i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
	{
		vkDestroyBuffer(logical_device, dyn_vertex_buffers[i].buffer, nullptr);
	}
	vkFreeMemory(logical_device, dyn_vertex_buffer_memory, nullptr);
}

unsigned char* VertexBufferDigress(int size, VkBuffer *buffer, VkDeviceSize *buffer_offset)
{
	dynbuffer_t* dyn_vb = &dyn_vertex_buffers[current_dyn_buffer_index];

	if ((dyn_vb->current_offset + size) > (DYNAMIC_VERTEX_BUFFER_SIZE_KB * 1024))
	{
		printf("Out of dynamic vertex buffer space, increase DYNAMIC_VERTEX_BUFFER_SIZE_KB \n");
		dyn_vb->current_offset = 0;
	}

	*buffer = dyn_vb->buffer;
	*buffer_offset = dyn_vb->current_offset;

	unsigned char *data = dyn_vb->data + dyn_vb->current_offset;
	dyn_vb->current_offset += size;

	return data;
}

void VertexBuffersAllocate()
{
	int i;

	printf("Initializing dynamic vertex buffers\n");

	VkResult err;

	VkBufferCreateInfo buffer_create_info;
	memset(&buffer_create_info, 0, sizeof(buffer_create_info));
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = DYNAMIC_VERTEX_BUFFER_SIZE_KB * 1024;
	buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	for (i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
	{
		dyn_vertex_buffers[i].current_offset = 0;

		err = vkCreateBuffer(logical_device, &buffer_create_info, nullptr, &dyn_vertex_buffers[i].buffer);
		if (err != VK_SUCCESS)
			printf("vkCreateBuffer failed \n");
	}

	VkMemoryRequirements memory_requirements;
	for (i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
	{
		vkGetBufferMemoryRequirements(logical_device, dyn_vertex_buffers[i].buffer, &memory_requirements);
	}

	const int align_mod = memory_requirements.size % memory_requirements.alignment;
	const int aligned_size = ((memory_requirements.size % memory_requirements.alignment) == 0)
	                         ? memory_requirements.size
	                         : (memory_requirements.size + memory_requirements.alignment - align_mod);

	VkMemoryAllocateInfo memory_allocate_info;
	memset(&memory_allocate_info, 0, sizeof(memory_allocate_info));
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = NUM_DYNAMIC_BUFFERS * aligned_size;
	memory_allocate_info.memoryTypeIndex = MemoryTypeFromProperties(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

	//num_vulkan_dynbuf_allocations += 1;
	err = vkAllocateMemory(logical_device, &memory_allocate_info, nullptr, &dyn_vertex_buffer_memory);
	if (err != VK_SUCCESS)
		printf("vkAllocateMemory failed \n");

	for (i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
	{
		err = vkBindBufferMemory(logical_device, dyn_vertex_buffers[i].buffer, dyn_vertex_buffer_memory, i * aligned_size);
		if (err != VK_SUCCESS)
			printf("vkBindBufferMemory failed \n");
	}

	void *data;
	err = vkMapMemory(logical_device, dyn_vertex_buffer_memory, 0, NUM_DYNAMIC_BUFFERS * aligned_size, 0, &data);
	if (err != VK_SUCCESS)
		printf("vkMapMemory failed \n");

	for (i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
		dyn_vertex_buffers[i].data = (unsigned char *)data + (i * aligned_size);
}

} //namespace control

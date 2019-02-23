#include "control.h"

/* {
GVAR: logical_device -> startup.cpp
} */

//{
VkCommandBuffer command_buffer;
VkPhysicalDeviceMemoryProperties memory_properties;
//}

static VkCommandPool command_pool;
static VkCommandBuffer *command_buffers = nullptr;
static int current_dyn_buffer_index = 0;
static VkDeviceMemory dyn_vertex_buffer_memory;
static dynbuffer_t dyn_vertex_buffers[NUM_DYNAMIC_BUFFERS];

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

	command_buffers = new VkCommandBuffer[count];

	VkResult result = vkAllocateCommandBuffers(logical_device, &command_buffer_allocate_info, &command_buffers[0]);
	if(result != VK_SUCCESS)
	{
		std::cout << "Could not allocate command buffers." << std::endl;
		return false;
	}
	command_buffer = command_buffers[0];
	return true;
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

bool CreateFence(VkFence &fence)
{
	VkFenceCreateInfo fenceInfo =
	{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,    	// VkStructureType            sType
		nullptr,                                    // const void               * pNext
		VK_FENCE_CREATE_SIGNALED_BIT               // VkSemaphoreCreateFlags     flags
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

inline int MemoryTypeFromProperties(uint32_t type_bits, VkFlags requirements_mask, VkFlags preferred_mask)
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

unsigned char* VertexAllocate(int size, VkBuffer *buffer, VkDeviceSize *buffer_offset)
{
	dynbuffer_t *dyn_vb = &dyn_vertex_buffers[current_dyn_buffer_index];

	if ((dyn_vb->current_offset + size) > (DYNAMIC_VERTEX_BUFFER_SIZE_KB * 1024))
		printf("Out of dynamic vertex buffer space, increase DYNAMIC_VERTEX_BUFFER_SIZE_KB \n");

	*buffer = dyn_vb->buffer;
	*buffer_offset = dyn_vb->current_offset;

	unsigned char *data = dyn_vb->data + dyn_vb->current_offset;
	dyn_vb->current_offset += size;

	return data;
}

void InitDynamicVertexBuffers()
{
	int i;

	printf("Initializing dynamic vertex buffers\n");

	VkResult err;

	VkBufferCreateInfo buffer_create_info;
	memset(&buffer_create_info, 0, sizeof(buffer_create_info));
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = DYNAMIC_VERTEX_BUFFER_SIZE_KB * 1024;
	buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	for (i = 0; i < NUM_DYNAMIC_BUFFERS; ++i)
	{
		dyn_vertex_buffers[i].current_offset = 0;

		err = vkCreateBuffer(logical_device, &buffer_create_info, NULL, &dyn_vertex_buffers[i].buffer);
		if (err != VK_SUCCESS)
			printf("vkCreateBuffer failed \n");
	}

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(logical_device, dyn_vertex_buffers[0].buffer, &memory_requirements);

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
	err = vkAllocateMemory(logical_device, &memory_allocate_info, NULL, &dyn_vertex_buffer_memory);
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
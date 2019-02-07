#include "control.h"

/* {
GVAR: logical_device -> startup.cpp
} */

//{
VkCommandBuffer command_buffer;
//}

static VkCommandPool command_pool;
static VkCommandBuffer *command_buffers = nullptr;

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

	if(command_buffers != nullptr)
	{
		delete command_buffers;
	}
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

	VkResult result = vkCreateSemaphore( logical_device, &semaphore_create_info, nullptr, &semaphore);
	if(result != VK_SUCCESS)
	{
		std::cout << "Could not create a semaphore." << std::endl;
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
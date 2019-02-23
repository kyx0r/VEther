#include "startup.h"
#include "swapchain.h"

extern VkCommandBuffer command_buffer;
extern VkPhysicalDeviceMemoryProperties	memory_properties;

//-----------------------------------
#define DYNAMIC_VERTEX_BUFFER_SIZE_KB	2048
#define NUM_DYNAMIC_BUFFERS 1

typedef struct
{
	VkBuffer			buffer;
	uint32_t			current_offset;
	unsigned char *		data;
} dynbuffer_t;

namespace control
{

bool CreateCommandPool(VkCommandPoolCreateFlags parameters, uint32_t queue_family);
void DestroyCommandPool();
bool AllocateCommandBuffers(VkCommandBufferLevel level, uint32_t count);
void FreeCommandBuffers(uint32_t count);
bool CreateSemaphore(VkSemaphore &semaphore);
bool CreateFence(VkFence &fence);
bool BeginCommandBufferRecordingOperation(VkCommandBufferUsageFlags usage, VkCommandBufferInheritanceInfo *secondary_command_buffer_info);
bool EndCommandBufferRecordingOperation();
bool ResetCommandPool(bool release_resources);
bool ResetCommandBuffer(bool release_resources);
void InitDynamicVertexBuffers();
unsigned char* VertexAllocate(int size, VkBuffer *buffer, VkDeviceSize *buffer_offset);

inline bool SubmitCommandBuffersToQueue(VkQueue queue, VkFence fence, VkSubmitInfo &submit_info)
{
	VkResult result = vkQueueSubmit(queue, 1, &submit_info, fence);
	if(result != VK_SUCCESS)
	{
		std::cout << "Error occurred during command buffer submission." << std::endl;
		return false;
	}
	return true;
}

inline bool WaitForAllSubmittedCommandsToBeFinished()
{
	VkResult result = vkDeviceWaitIdle(logical_device);
	if(result != VK_SUCCESS)
	{
		std::cout << "Waiting on a device failed." << std::endl;
		return false;
	}
	return true;
}

} //namespace control

//-----------------------------------
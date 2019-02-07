#include "startup.h"
#include "swapchain.h"

extern VkCommandBuffer command_buffer;

//-----------------------------------

bool CreateCommandPool(VkCommandPoolCreateFlags parameters, uint32_t queue_family);
bool AllocateCommandBuffers(VkCommandBufferLevel level, uint32_t count);
bool CreateSemaphore(VkSemaphore &semaphore);
bool BeginCommandBufferRecordingOperation(VkCommandBufferUsageFlags usage, VkCommandBufferInheritanceInfo *secondary_command_buffer_info);
bool EndCommandBufferRecordingOperation();
bool ResetCommandPool(bool release_resources);
bool ResetCommandBuffer(bool release_resources);

inline void SetImageMemoryBarrier(VkPipelineStageFlags generating_stages, VkPipelineStageFlags consuming_stages, VkImageMemoryBarrier &image_memory_barrier)
{
	vkCmdPipelineBarrier(command_buffer, generating_stages, consuming_stages, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
}

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

//-----------------------------------
#include "startup.h"
#include "swapchain.h"

//extern VkCommandBuffer *command_buffers;
extern VkCommandBuffer command_buffer;

bool CreateCommandPool(VkCommandPoolCreateFlags parameters, uint32_t queue_family);
bool AllocateCommandBuffers(VkCommandBufferLevel level, uint32_t count);
bool CreateSemaphore(VkSemaphore &semaphore);
bool BeginCommandBufferRecordingOperation(VkCommandBufferUsageFlags usage, VkCommandBufferInheritanceInfo *secondary_command_buffer_info);
bool EndCommandBufferRecordingOperation();
bool ResetCommandBuffer(bool release_resources);

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
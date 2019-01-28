#include "window.h"
#include "control.h"

GLFWwindow* window;
VkSwapchainKHR swapchain = VK_NULL_HANDLE;
VkSwapchainKHR old_swapchain = VK_NULL_HANDLE;
VkSemaphore AcquiredSemaphore;
VkSemaphore ReadySemaphore;

void initWindow() 
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(640, 480, "VEther", nullptr, nullptr);
}

inline bool Draw()
{
	uint32_t image_index;
	WaitForAllSubmittedCommandsToBeFinished();
	if(
		!AcquireSwapchainImage(swapchain, AcquiredSemaphore, VK_NULL_HANDLE, image_index)||
		!BeginCommandBufferRecordingOperation(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr)
		)
	{
		return false;
	}
	return true;
}

void mainLoop() 
{
    while (!glfwWindowShouldClose(window))
	{
		if(!Draw())
		{
			std::cout << "Critical Error! Abandon the ship." << std::endl;
			break;
		}
        glfwPollEvents();
    }
}
#include "startup.h"

extern GLFWwindow* window;
extern VkSwapchainKHR swapchain;
extern VkSwapchainKHR old_swapchain;
extern VkSemaphore AcquiredSemaphore;
extern VkSemaphore ReadySemaphore;
extern VkQueue GraphicsQueue;
extern VkQueue ComputeQueue;

//------------------------

void initWindow();
void mainLoop();
//bool Draw();

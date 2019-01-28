#include "startup.h"

extern GLFWwindow* window;
extern VkSwapchainKHR swapchain;
extern VkSwapchainKHR old_swapchain;
extern VkSemaphore AcquiredSemaphore;
extern VkSemaphore ReadySemaphore;

//------------------------

void initWindow();
void mainLoop();
//bool Draw();

#include "startup.h"

extern GLFWwindow* window;
extern VkSwapchainKHR swapchain;
extern VkSwapchainKHR old_swapchain;
extern VkSemaphore AcquiredSemaphore;
extern VkSemaphore ReadySemaphore;
extern VkQueue GraphicsQueue;
extern VkQueue ComputeQueue;
extern int window_width;
extern int window_height;
extern uint32_t graphics_queue_family_index;
extern uint32_t compute_queue_family_index;

//------------------------

void initWindow();
void mainLoop();

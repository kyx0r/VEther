#include "startup.h"

extern GLFWwindow* _window;
extern VkSwapchainKHR _swapchain;
extern VkSwapchainKHR old_swapchain;
extern VkSemaphore AcquiredSemaphore;
extern VkSemaphore ReadySemaphore;
extern VkFence Fence_one;
extern VkQueue GraphicsQueue;
extern VkQueue ComputeQueue;
extern int window_width;
extern int window_height;
extern uint32_t graphics_queue_family_index;
extern uint32_t compute_queue_family_index;
extern double time1;
extern double y_wheel;
extern double xm_norm;
extern double ym_norm;
extern double frametime;
//------------------------

namespace window
{

void initWindow();
void mainLoop();

} //namespace window

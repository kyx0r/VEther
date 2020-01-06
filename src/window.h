#ifndef __WINDOW_H__
#define __WINDOW_H__
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
extern int32_t xm;
extern int32_t ym;
extern double frametime;
extern double lastfps;
//------------------------

namespace window
{

void initWindow();
void PreDraw();
void mainLoop();

} //namespace window
#endif

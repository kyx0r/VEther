#include "window.h"
#include "surface.h"

extern VkImage *handle_array_of_swapchain_images;
extern uint32_t number_of_swapchain_images;
extern VkFormat image_format;

namespace swapchain
{

//-----------------------------------

bool SelectNumberOfSwapchainImages();

bool SetSizeOfSwapchainImages(uint32_t x, uint32_t y);

bool ComputeSizeOfSwapchainImages(uint32_t x, uint32_t y);

bool SelectDesiredUsageScenariosOfSwapchainImages(VkImageUsageFlags desired_usages);

//how the image is transformed when rendering.
bool SelectTransformationOfSwapchainImages(VkSurfaceTransformFlagBitsKHR desired_transform);

//set surface format and its matching color format
bool SelectFormatOfSwapchainImages(VkSurfaceFormatKHR desired_surface_format);

bool CreateSwapchain(VkSwapchainKHR &_swapchain, VkSwapchainKHR &old_swapchain);

void DestroySwapchain(VkSwapchainKHR &_swapchain);

bool GetHandlesOfSwapchainImages(VkSwapchainKHR &_swapchain);

uint8_t AcquireSwapchainImage(VkSwapchainKHR _swapchain, VkSemaphore semaphore, VkFence fence, uint32_t &image_index);

//------------------------------------

} //namespace swapchain

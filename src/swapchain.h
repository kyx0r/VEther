#include "window.h"
#include "surface.h"

extern VkImage *handle_array_of_swapchain_images;

//-----------------------------------

bool SelectNumberOfSwapchainImages();

bool ComputeSizeOfSwapchainImages(uint32_t x, uint32_t y);

bool SelectDesiredUsageScenariosOfSwapchainImages(VkImageUsageFlags desired_usages); 

//how the image is transformed when rendering. 
bool SelectTransformationOfSwapchainImages(VkSurfaceTransformFlagBitsKHR desired_transform);

//set surface format and its matching color format 
bool SelectFormatOfSwapchainImages(VkSurfaceFormatKHR desired_surface_format);

bool CreateSwapchain(VkSwapchainKHR &swapchain, VkSwapchainKHR &old_swapchain);

bool GetHandlesOfSwapchainImages(VkSwapchainKHR &swapchain);

bool AcquireSwapchainImage(VkSwapchainKHR swapchain, VkSemaphore semaphore, VkFence fence, uint32_t &image_index);

//------------------------------------
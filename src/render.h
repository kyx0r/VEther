#include "startup.h"
#include "swapchain.h"

extern VkRenderPass renderPasses[100];
extern VkFramebuffer framebuffers[100];

//-----------------------
void CreateFramebuffer(VkImageView colorView, VkImageView depthView, uint32_t width, uint32_t height);
void CreateRenderPass(VkFormat depthFormat, bool late);
void DestroyRenderPass(int render_index);
void DestroyFramebuffer(int buffer_index);
void CreateImageView(VkImage image, VkFormat format, uint32_t mipLevel, uint32_t levelCount);
void StartRenderPass(VkRect2D render_area, VkClearValue *clear_values, VkSubpassContents subpass_contents, int render_index, int buffer_index);
//-----------------------
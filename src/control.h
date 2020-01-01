#include "startup.h"
#include "swapchain.h"

//-----------------------------------
#define DYNAMIC_VERTEX_BUFFER_SIZE_KB	4096
#define DYNAMIC_UNIFORM_BUFFER_SIZE_KB  1024
#define DYNAMIC_INDEX_BUFFER_SIZE_KB    2048
#define MAX_UNIFORM_ALLOC		2048
#define NUM_DYNAMIC_BUFFERS 2
#define NUM_STAGING_BUFFERS 5 //in actuality there are 2 staging as command buffer 0 is primary renderer.
#define TEXTURE_MAX_HEAPS 5
#define NUM_COMMAND_BUFFERS 5

extern VkCommandBuffer command_buffer;
extern VkCommandBuffer scommand_buffer;
extern VkPhysicalDeviceMemoryProperties	memory_properties;
extern VkDescriptorPool descriptor_pool;
extern VkDescriptorSetLayout vubo_dsl;
extern VkDescriptorSetLayout fubo_dsl;
extern VkDescriptorSetLayout tex_dsl;
extern uint8_t current_cmd_buffer_index;
extern int current_dyn_buffer_index;
extern int current_staging_buffer;
//-----------------------------------

typedef struct vram_heap
{
	VkDeviceMemory	memory;
	VkDeviceSize offset;
	VkDeviceSize size;
	struct vram_heap* prev;
	struct vram_heap* next;
	bool free;
} vram_heap;
extern vram_heap texmgr_heaps[TEXTURE_MAX_HEAPS];

typedef struct
{
	VkBuffer			buffer;
	uint32_t			current_offset;
	unsigned char*		data;
} dynbuffer_t;
extern dynbuffer_t dyn_index_buffers[NUM_DYNAMIC_BUFFERS];
extern dynbuffer_t dyn_vertex_buffers[NUM_DYNAMIC_BUFFERS];
extern dynbuffer_t dyn_uniform_buffers[NUM_DYNAMIC_BUFFERS];

typedef struct
{
	VkBuffer			buffer;
	VkCommandBuffer		command_buffer;
	VkFence				fence;
	int					current_offset;
	bool				submitted;
	unsigned char *		data;
} stagingbuffer_t;
extern stagingbuffer_t	staging_buffers[NUM_STAGING_BUFFERS];

namespace control
{

bool CreateCommandPool(VkCommandPoolCreateFlags parameters, uint32_t queue_family);
void DestroyCommandPool();
void DestroyDynBuffers();
void DestroyStagingBuffers();
void DestroyUniformBuffers();
void DestroyIndexBuffers();
void DestroyVramHeaps();
bool AllocatePrimaryCommandBuffers(uint32_t count);
bool AllocateSecondaryCommandBuffers(uint32_t count);
void FreeCommandBuffers(uint32_t count);
bool CreateSemaphore(VkSemaphore &semaphore);
bool CreateFence(VkFence &fence, VkFenceCreateFlags flags);
void SetCommandBuffer(uint8_t i);
void SetSCommandBuffer(uint8_t i);
void BeginCommandBufferRecordingOperation(VkCommandBufferUsageFlags usage, VkCommandBufferInheritanceInfo *secondary_command_buffer_info);
bool ResetCommandPool(bool release_resources);
bool ResetCommandBuffer(bool release_resources);
void CreateDescriptorPool();
int MemoryTypeFromProperties(uint32_t type_bits, VkFlags requirements_mask, VkFlags preferred_mask);

void VramHeapAllocate(VkDeviceSize size, uint32_t memory_type_index);
void IndexBuffersAllocate();
void VertexBuffersAllocate();
void StagingBuffersAllocate();
void UniformBuffersAllocate();

vram_heap* VramHeapDigress(VkDeviceSize size, VkDeviceSize alignment, VkDeviceSize* aligned_offset);
unsigned char* IndexBufferDigress(int size, VkBuffer* buffer, VkDeviceSize* buffer_offset);
unsigned char* VertexBufferDigress(int size, VkBuffer *buffer, VkDeviceSize *buffer_offset);
unsigned char* StagingBufferDigress(int size, int alignment);
unsigned char* UniformBufferDigress(int size, VkBuffer* buffer, uint32_t* buffer_offset, VkDescriptorSet* descriptor_set, int index);

void FlushDynamicBuffers();
void InvalidateDynamicBuffers();

void SubmitStagingBuffer();
void ResetStagingBuffer();



} //namespace control

//-----------------------------------

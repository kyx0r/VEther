#include <cstdio>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>
#include <tchar.h>
#include <string>

#if defined _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#define VK_NO_PROTOTYPES

//#define GLFW_INCLUDE_VULKAN //<- this define will enable similar functionality but in glfw library
// it might have some features with easier setup, but not sure if they are needed, 
//since im implementing parts of this library in my own code already. 
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h> //<- this header allows me to access the low level window handles. 
#include <vulkan.h>
//------------------
#pragma once
#include "vulkan_decl.h"

#define DEBUG

//--------------------

extern VkDevice logical_device;
extern VkInstance instance;
extern VkPhysicalDevice target_device;

extern uint32_t queue_families_count;
struct QueueInfo
{
	uint32_t FamilyIndex;
	float *Priorities;
};

//--------------------

//-------------------- funcs
void debug_pause();
const char* GetVulkanResultString(VkResult result);
void ReleaseVulkanLoaderLibrary();
bool LoadVulkan();
bool LoadVulkanGlobalFuncs();
bool LoadInstanceFunctions();
bool LoadDeviceLevelFunctions();
bool CheckInstanceExtensions();
bool CheckPhysicalDeviceExtensions();
bool CheckPhysicalDevices();
bool CheckQueueProperties(VkQueueFlags desired_capabilities,  uint32_t &queue_family_index);
bool IsExtensionSupported(const char* extension);
bool SetQueue(QueueInfo *array, uint32_t family, float *_Priorities, int index);
bool CreateVulkanInstance(uint32_t count, const char** exts); 
bool CreateLogicalDevice(QueueInfo *array, int number_of_queues, uint32_t ext_count, const char** exts);

//--------------------


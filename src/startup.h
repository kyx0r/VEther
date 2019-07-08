#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>
#include <string>
#include <assert.h>
#include <chrono>
#include <thread>
#include <cfloat>

#if defined _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_EXPOSE_NATIVE_WIN32
#include <tchar.h>
#endif

#if defined __linux
#define VK_USE_PLATFORM_XCB_KHR
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
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

#define VK_CHECK(call) \
	do { \
		VkResult result_ = call; \
		assert(result_ == VK_SUCCESS); \
	} while (0)

#ifndef ARRAYSIZE
#define ARRAYSIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

#define	CLAMP(_minval, x, _maxval)		\
	((x) < (_minval) ? (_minval) :		\
	 (x) > (_maxval) ? (_maxval) : (x))

#define	q_min(a, b)	(((a) < (b)) ? (a) : (b))
#define	q_max(a, b)	(((a) > (b)) ? (a) : (b))

#define DEFAULT_MEMORY (256 * 1024 * 1024)


//--------------------

extern VkDevice logical_device;
extern VkInstance instance;
extern VkPhysicalDevice target_device;
extern uint32_t max2DTex_size;

extern uint32_t queue_families_count;
struct QueueInfo
{
	uint32_t FamilyIndex;
	float *Priorities;
};

//--------------------

namespace startup
{

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
bool CreateVulkanInstance(uint32_t count, const char** exts);
bool CreateLogicalDevice(QueueInfo *array, int number_of_queues, uint32_t ext_count, const char** exts);
VkDebugReportCallbackEXT registerDebugCallback();

inline bool SetQueue(QueueInfo *array, uint32_t family, float *_Priorities, int index)
{
	array[index].FamilyIndex = family;
	array[index].Priorities = _Priorities;
	return true;
}

//--------------------

} //namespace startup

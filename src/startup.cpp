#include "startup.h"
#include "zone.h"
#include "flog.h"

/* {
   GVAR: PFN_* -> vulkan_decl.h
   } */

static VkAllocationCallbacks allocator;

/*    Warning! if making a 32 bit build, set allocators to null and remove the respectable
   structures, as there is a driver bug! Else we will crash hard.  */

//{
VkInstance instance;
VkPhysicalDevice target_device = VK_NULL_HANDLE;
VkDevice logical_device;
VkAllocationCallbacks* allocators = &allocator;
uint32_t max2DTex_size = 0;
uint32_t queue_families_count = 0;
//}

static uint32_t extensions_count = 0;
static uint32_t device_count = 0;
static uint32_t desired_count = 0;
static const char** desired_extensions = nullptr;
static VkExtensionProperties* available_extensions = nullptr;
static VkPhysicalDevice* available_devices = nullptr;
static VkPhysicalDeviceFeatures device_features;
static VkPhysicalDeviceProperties device_properties;

#ifdef DEBUG
static int d_layers_count = 1;
const char* debugLayers[] =
{
	"VK_LAYER_KHRONOS_validation"
};
#endif

#if defined _WIN32
static HMODULE vulkan_lib;
#elif defined __linux
#include <dlfcn.h>
static void* vulkan_lib;
#endif

namespace startup
{
//---------------------------------------------------------------------

void debug_pause()
{
	printf("Press any key to continue... \n");
	char buf[10];
	fgets(buf, 10, stdin);
	exit(1);
}

const char* GetVulkanResultString(VkResult result)
{
	switch (result)
	{
	case VK_SUCCESS:
		return "Success";
	case VK_NOT_READY:
		return "A fence or query has not yet completed";
	case VK_TIMEOUT:
		return "A wait operation has not completed in the specified time";
	case VK_EVENT_SET:
		return "An event is signaled";
	case VK_EVENT_RESET:
		return "An event is unsignaled";
	case VK_INCOMPLETE:
		return "A return array was too small for the result";
	case VK_ERROR_OUT_OF_HOST_MEMORY:
		return "A host memory allocation has failed";
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		return "A device memory allocation has failed";
	case VK_ERROR_INITIALIZATION_FAILED:
		return "Initialization of an object could not be completed for implementation-specific reasons";
	case VK_ERROR_DEVICE_LOST:
		return "The logical or physical device has been lost";
	case VK_ERROR_MEMORY_MAP_FAILED:
		return "Mapping of a memory object has failed";
	case VK_ERROR_LAYER_NOT_PRESENT:
		return "A requested layer is not present or could not be loaded";
	case VK_ERROR_EXTENSION_NOT_PRESENT:
		return "A requested extension is not supported";
	case VK_ERROR_FEATURE_NOT_PRESENT:
		return "A requested feature is not supported";
	case VK_ERROR_INCOMPATIBLE_DRIVER:
		return "The requested version of Vulkan is not supported by the driver or is otherwise incompatible";
	case VK_ERROR_TOO_MANY_OBJECTS:
		return "Too many objects of the type have already been created";
	case VK_ERROR_FORMAT_NOT_SUPPORTED:
		return "A requested format is not supported on this device";
	case VK_ERROR_SURFACE_LOST_KHR:
		return "A surface is no longer available";
	case VK_SUBOPTIMAL_KHR:
		return "A swapchain no longer matches the surface properties exactly, but can still be used";
	case VK_ERROR_OUT_OF_DATE_KHR:
		return "A surface has changed in such a way that it is no longer compatible with the swapchain";
	case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
		return "The display used by a swapchain does not use the same presentable image layout";
	case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
		return "The requested window is already connected to a VkSurfaceKHR, or to some other non-Vulkan API";
	case VK_ERROR_VALIDATION_FAILED_EXT:
		return "A validation layer found an error";
	default:
		return "ERROR: UNKNOWN VULKAN ERROR";
	}
}

#ifdef DEBUG

static VkBool32 VKAPI_CALL debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
	// This silences warnings like "For optimal performance image layout should be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL instead of GENERAL."
	// We'll assume other performance warnings are also not useful.
	if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		return VK_FALSE;

	const char* type =
	    (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	    ? "ERROR"
	    : (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	    ? "WARNING"
	    : "INFO";

	char message[4096];
	snprintf(message, ARRAYSIZE(message), "%s: %s\n", type, pMessage);

	fatal("%s", message);

#ifdef _WIN32
	OutputDebugStringA(message);
#endif

	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		printf("\nValidation error encountered! \n");

	if(flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		return VK_FALSE;
	debug_pause();
	return VK_FALSE;
}

static bool CheckValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	VkLayerProperties availableLayers[layerCount];
	vkEnumerateInstanceLayerProperties(&layerCount, &availableLayers[0]);

	for(int i = 0; i<d_layers_count; i++)
	{
		bool layerFound = false;
		for (uint32_t z = 0; z<layerCount; z++)
		{
			if (strcmp(debugLayers[i], (char*) &availableLayers[z]) == 0)
			{
				layerFound = true;
				break;
			}
		}
		if (!layerFound)
		{
			return false;
		}
	}
	return true;
}

VkDebugReportCallbackEXT registerDebugCallback()
{
	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT |
	                   VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
	                   VK_DEBUG_REPORT_ERROR_BIT_EXT;
	createInfo.pfnCallback = debugReportCallback;

	VkDebugReportCallbackEXT callback = 0;
	VK_CHECK(vkCreateDebugReportCallbackEXT(instance, &createInfo, 0, &callback));

	return callback;
}

#endif

bool LoadVulkan()
{
#if defined _WIN32
#define libtype HMODULE
	vulkan_lib = LoadLibrary("vulkan-1.dll");
#define LoadFunction GetProcAddress
#elif defined __linux
#define libtype void*
	vulkan_lib = dlopen("libvulkan.so.1", RTLD_NOW);
#define LoadFunction dlsym
#endif
	if(vulkan_lib == nullptr)
	{
		fatal("Failed to load the Vulkan Runtime Library! ");
		return false;
	}

#define EXPORTED_VULKAN_FUNCTION( name )				\
    name = (PFN_##name) (LoadFunction(vulkan_lib, #name));		\
    if(name == nullptr)							\
      {									\
	warn("Could not load exported Vulkan function: %s", #name);	\
	return false;							\
      }else	{							\
      info("Exported Vulkan function: %s" #name);}			\

#include "vulkan_functions_list.inl"

	return true;
}

bool LoadVulkanGlobalFuncs()
{
#define GLOBAL_LEVEL_VULKAN_FUNCTION( name )				\
    name = (PFN_##name) (vkGetInstanceProcAddr( nullptr, #name));	\
    if(name == nullptr)							\
      {									\
	warn("Could not load global Vulkan function: %s", #name);	\
	return false;							\
      }else								\
      {info("Loaded global Vulkan function: %s", #name);}	\

#include "vulkan_functions_list.inl"
	return true;
}

//Instance functions take 1st parameter of type Vkdevice || VkQueue || VkCommandBuffer
bool LoadInstanceFunctions()
{
#define INSTANCE_LEVEL_VULKAN_FUNCTION( name )				\
    name = (PFN_##name)vkGetInstanceProcAddr( instance, #name);		\
    if(name == nullptr)							\
      {									\
	warn("Could not load instance-level Vulkan function: %s", #name); \
	return false;							\
      }else								\
      {info("Loaded instance-level Vulkan function: %s", #name);}	\

#include "vulkan_functions_list.inl"

	// Load instance-level functions from enabled extensions
#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION( name, extension ) \
    for(uint32_t i = 0; i<desired_count; i++) {				\
      if( std::string( desired_extensions[i] ) == std::string( extension ) ) { \
        name = (PFN_##name)vkGetInstanceProcAddr( instance, #name );	\
        if( name == nullptr ){						\
		warn("Could not load instance-level Vulkan function named: %s", #name);  \
		return false;                                                        \
  }else                                                                       \
  {info("Loaded function from extension: %s", #name);}		\
}                                                                         \
}

#include "vulkan_functions_list.inl"

	return true;
}

bool CheckInstanceExtensions()
{
	VkResult result = VK_SUCCESS;
	result = vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr);
	if(result != VK_SUCCESS || extensions_count == 0)
	{
		fatal("Could not get the extension count!");
		return false;
	}
	char* mem = (char*) zone::Z_Malloc(sizeof(VkExtensionProperties) * extensions_count);
	available_extensions = new(mem) VkExtensionProperties [extensions_count];
	result = vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, &available_extensions[0]);
	if(result != VK_SUCCESS || extensions_count == 0)
	{
		fatal("Could not enumerate extensions!  ");
		return false;
	}
	return true;
}

bool IsExtensionSupported(const char* extension)
{
	for(uint32_t i = 0; i<extensions_count; i++)
	{
		if( strstr( (char*) &available_extensions[i], extension ) )
		{
			return true;
		}
	}
	debug("Available Extensions: ");
	for(uint32_t i = 0; i<extensions_count; i++)
	{
		debug("%s", (char*)&available_extensions[i]);
	}
	return false;
}

bool CreateVulkanInstance(uint32_t count, const char** exts)
{
	desired_count = count;
	desired_extensions = exts;
	for(uint32_t i = 0; i < count; i++)
	{
		if(desired_extensions[i] != nullptr)
		{
			if(!IsExtensionSupported(desired_extensions[i]))
			{
				warn("Extension %s  is not supported!", desired_extensions[i]);
				return false;
			}
			else
			{
				info("Using instance extension: %s ", desired_extensions[i]);
			}
		}
	}

	VkApplicationInfo app_info =
	{
		VK_STRUCTURE_TYPE_APPLICATION_INFO,
		nullptr,
		"Vether",
		VK_MAKE_VERSION(1,0,0),
		"Vether",
		VK_MAKE_VERSION(1,0,0),
		VK_MAKE_VERSION(1,0,0)
	};

	VkInstanceCreateInfo instance_create_info;
	memset(&instance_create_info, 0, sizeof(instance_create_info));
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pApplicationInfo = &app_info;
	instance_create_info.enabledExtensionCount = count;
	instance_create_info.ppEnabledExtensionNames = &desired_extensions[0];

#ifdef DEBUG
	if(!CheckValidationLayerSupport())
	{
		warn("Debug Layers are not found! ");
	}
	else
	{
		instance_create_info.enabledLayerCount = d_layers_count;
		instance_create_info.ppEnabledLayerNames = debugLayers;
	}
#endif

	//define custom vulkan allocators
	//allocators = nullptr;
	allocators->pUserData = nullptr;
	allocators->pfnAllocation = (PFN_vkAllocationFunction)&VEtherAlloc;
	allocators->pfnReallocation = (PFN_vkReallocationFunction)&VEtherRealloc;
	allocators->pfnFree = (PFN_vkFreeFunction)&VEtherFree;
	allocators->pfnInternalAllocation = nullptr;
	allocators->pfnInternalFree = nullptr;

	VkResult result = VK_SUCCESS;
	result = vkCreateInstance(&instance_create_info, allocators, &instance);
	if(result != VK_SUCCESS)
	{
		fatal("Could not create Vulkan Instance!  ");
		return false;
	}

	//instance is created and we wont dont need to store instance extensions here anymore.
	//next step will be to load device extensions in this array.
	zone::Z_Free((char*)&available_extensions[0]);
	available_extensions = nullptr;

	return true;
}

bool CheckPhysicalDevices()
{
	VkResult result = VK_SUCCESS;
	result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
	if(result != VK_SUCCESS || device_count == 0)
	{
		fatal("Could not get number of physical devices!  ");
		return false;
	}
	char* mem = (char*) zone::Z_Malloc(sizeof(VkPhysicalDevice) * device_count);
	available_devices = new(mem) VkPhysicalDevice [device_count];
	result = vkEnumeratePhysicalDevices(instance, &device_count, &available_devices[0]);
	if(result != VK_SUCCESS || device_count == 0)
	{
		fatal("Could not enumerate physical devices!  ");
		return false;
	}
	return true;
}

bool CheckPhysicalDeviceExtensions()
{
	VkResult result = VK_SUCCESS;
	uint32_t device_extensions_count = 0;
	for(uint32_t i = 0; i<device_count; i++)
	{
		result = vkEnumerateDeviceExtensionProperties(available_devices[i], nullptr, &device_extensions_count, nullptr);
		if(result != VK_SUCCESS || device_extensions_count == 0)
		{
			fatal("Could not get number of physical device extensions!  ");
			return false;
		}

		VkExtensionProperties m[device_extensions_count];

		result = vkEnumerateDeviceExtensionProperties(available_devices[i], nullptr, &device_extensions_count, &m[0]);
		if(result != VK_SUCCESS || device_extensions_count == 0)
		{
			fatal("Could not enumerate device extensions!  ");
			return false;
		}

		vkGetPhysicalDeviceFeatures(available_devices[i], &device_features);
		vkGetPhysicalDeviceProperties(available_devices[i], &device_properties);

		if(device_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && device_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			continue;
		}
		else
		{
			char* mem = (char*) zone::Z_Malloc(sizeof(VkExtensionProperties) * device_extensions_count);
			memcpy(mem, m, (sizeof(VkExtensionProperties) * device_extensions_count));
			available_extensions = new(mem) VkExtensionProperties [device_extensions_count];

			device_features = {};
			max2DTex_size = device_properties.limits.maxImageDimension2D;
			target_device = available_devices[i];
			break;
		}

	}
	if(target_device == 0)
	{
		fatal("Could not find matching Gpu! ");
		return false;
	}
	extensions_count = device_extensions_count;
	zone::Z_Free((char*)&available_devices[0]);
	return true;
}

bool CheckQueueProperties(VkQueueFlags desired_capabilities,  uint32_t &queue_family_index )
{
	vkGetPhysicalDeviceQueueFamilyProperties(target_device, &queue_families_count, nullptr);
	if(queue_families_count == 0)
	{
		fatal("Could not get number of family queues!  ");
		return false;
	}
	VkQueueFamilyProperties queue_families[queue_families_count];
	vkGetPhysicalDeviceQueueFamilyProperties(target_device, &queue_families_count, &queue_families[0]);
	if(queue_families_count == 0)
	{
		fatal("Could not get properties of family queues!  ");
		return false;
	}
	for(uint32_t i = 0; i<queue_families_count; ++i)
	{
		if(queue_families[i].queueCount > 0 && (queue_families[i].queueFlags & desired_capabilities) == desired_capabilities)
		{
			queue_family_index = i;
			return true;
		}
	}
	warn("Some capabilities were of family queues not detected!");
	return false;
}

bool CreateLogicalDevice(QueueInfo *array, int number_of_queues, uint32_t ext_count, const char** exts)
{
	desired_count = ext_count;
	desired_extensions = exts;
	for(uint32_t i = 0; i < ext_count; i++)
	{
		if(desired_extensions[i] != nullptr)
		{
			if(!IsExtensionSupported(desired_extensions[i]))
			{
				fatal("Extension %s is no supported by physical device!", desired_extensions[i]);
				return false;
			}
		}
	}

	VkDeviceQueueCreateInfo queue_create_infos[number_of_queues] = {};
	for(int i = 0; i<number_of_queues; i++)
	{
		queue_create_infos[i] =
		{
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			nullptr,
			0,
			array[i].FamilyIndex,
			1,
			array[i].Priorities
		};
	}

	device_features.tessellationShader = true;
	device_features.fillModeNonSolid = true;

	VkDeviceCreateInfo device_create_info;
	memset(&device_create_info, 0, sizeof(device_create_info));
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queueCreateInfoCount = number_of_queues;
	device_create_info.pQueueCreateInfos = &queue_create_infos[0];
	device_create_info.enabledExtensionCount = ext_count;
	device_create_info.ppEnabledExtensionNames = &desired_extensions[0];
	device_create_info.pEnabledFeatures = &device_features;

	VkResult result = vkCreateDevice(target_device, &device_create_info, allocators, &logical_device);
	if(result != VK_SUCCESS || logical_device == VK_NULL_HANDLE)
	{
		fatal("Could not create logical device.");
		return false;
	}

	zone::Z_Free((char*)&available_extensions[0]);
	available_extensions = nullptr;
	return true;
}

bool LoadDeviceLevelFunctions()
{
	// Load core Vulkan API device-level functions
#define DEVICE_LEVEL_VULKAN_FUNCTION( name )				\
  name = (PFN_##name)vkGetDeviceProcAddr( logical_device, #name );	\
  if( name == nullptr ) {						\
	  warn("Could not load device-level Vulkan function named: %s",#name); \
    return false;							\
  }else	{								\
  info("Loaded device-level function: %s",#name);} \

#include "vulkan_functions_list.inl"

	// Load device-level functions from enabled extensions
#define DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION( name, extension )	\
  for(uint32_t i = 0; i<desired_count; i++) {				\
    if( std::string( desired_extensions[i] ) == std::string( extension ) ) { \
      name = (PFN_##name)vkGetDeviceProcAddr( logical_device, #name );	\
      if( name == nullptr ) {						\
	      warn("Could not load device-level Vulkan function from extension: %s", #name); \
	return false;							\
      }else{								\
	  info("Loaded device-level function from extension: %s", #name);}	\
    }									\
  }
#include "vulkan_functions_list.inl"
	return true;
}

void ReleaseVulkanLoaderLibrary()
{
	vkDestroyDevice(logical_device, allocators);
	vkDestroyInstance(instance, allocators);

	instance = VK_NULL_HANDLE;
	logical_device = VK_NULL_HANDLE;

	if(nullptr != vulkan_lib)
	{
#if defined _WIN32
		FreeLibrary(vulkan_lib);
#elif defined __linux
		dlclose(vulkan_lib);
#endif
		vulkan_lib = nullptr;
	}
}

} //namespace startup

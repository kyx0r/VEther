#include "shaders.h"
#include "flog.h"
#include "zone.h"
#include "render.h"

/* {
GVAR: logical_device -> startup.cpp
GVAR: VkShaderModule _shaders[20] -> /standalone/standalone.cpp
GVAR: uint32_t cur_shader_index -> /standalone/standalone.cpp
} */

static VkPipelineCache pipelineCache = 0;

namespace shaders
{
  
  static char* sfilenames[][1] =
    {
     {"./res/shaders/triangle.frag.glsl"},
     {"./res/shaders/triangle.vert.glsl"},
     {"./res/shaders/screen.frag.glsl"},
     {"./res/shaders/screen.vert.glsl"},
     {"./res/shaders/col.frag.glsl"}
    };
  
  static char* shaders[][5] =
    {
     {"0", sfilenames[0][0], "-V", "-o", "./res/shaders/triangle.frag.spv"},
     {"1", sfilenames[1][0], "-V", "-o", "./res/shaders/triangle.vert.spv"},
     {"2", sfilenames[2][0], "-V", "-o", "./res/shaders/screen.frag.spv"},
     {"3", sfilenames[3][0], "-V", "-o", "./res/shaders/screen.vert.spv"},
     {"4", sfilenames[4][0], "-V", "-o", "./res/shaders/col.frag.spv"}
    };
  
class UpdateListener : public FW::FileWatchListener
{
public:
	UpdateListener() {}
	void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action)
	{
	     info("Reloading shader file: %s", &dir[0]);

	     // Reload everything beacuase doing this with single shoot does not work
	     // for some weird reason and the code grows in complexity as well.
	     // now if there are many shaders, then this does not scale at all in terms of performace.
	     
	     VK_CHECK(vkDeviceWaitIdle(logical_device));
	     DestroyShaders();
	     cur_shader_index = 0;
	     render::DestroyPipeLines();
	     pipelineCount = 0;
	     for(uint32_t i = 0; i<ARRAYSIZE(shaders); ++i)
	       {
	     	 GLSL_COMPILER_ENTRY(ARRAYSIZE(shaders[i]), shaders[i]);
	       }
	     LoadShaders();
	}
};

FW::FileWatcher* fileWatcher;
	         
//this must be called only when logical device is initialized already.
void CompileShaders()
{
	fileWatcher = new FW::FileWatcher(); //calls Z_Malloc()
	for(uint32_t i = 0; i<ARRAYSIZE(shaders); ++i)
	{
		GLSL_COMPILER_ENTRY(ARRAYSIZE(shaders[i]), shaders[i]);
		// add a watch to the system
		fileWatcher->addWatch(sfilenames[i][0], new UpdateListener(), true);		
	}
       	
	trace("SHADERS COMPILED. \n");
}

VkShaderModule loadShaderMem(int index)
{
	return _shaders[index];
}

  void LoadShaders()
  {
	VkShaderModule triangleFS = shaders::loadShaderMem(0);
	VkShaderModule triangleVS = shaders::loadShaderMem(1);
	VkShaderModule screenFS = shaders::loadShaderMem(2);
	VkShaderModule screenVS = shaders::loadShaderMem(3);
	VkShaderModule colFS = shaders::loadShaderMem(4);     

	render::CreateGraphicsPipeline(pipelineCache, render::BasicTrianglePipe, 0, triangleVS, triangleFS);
	render::CreateGraphicsPipeline(pipelineCache, render::ScreenPipe, 0, screenVS, screenFS);
	render::CreateGraphicsPipeline(pipelineCache, render::ScreenPipe, 0, screenVS, colFS);

  }

void DestroyShaders()
  {
    VkShaderModule triangleFS = shaders::loadShaderMem(0);
    ASSERT(triangleFS, "Failed to load triangleFS.");
    VkShaderModule triangleVS = shaders::loadShaderMem(1);
    ASSERT(triangleVS, "Failed to load triangleVS.");
    VkShaderModule screenFS = shaders::loadShaderMem(2);
    ASSERT(screenFS, "Failed to load screenFS.");
    VkShaderModule screenVS = shaders::loadShaderMem(3);
    ASSERT(screenVS, "Failed to load screenVS.");
    VkShaderModule colFS = shaders::loadShaderMem(4);
    ASSERT(colFS, "Failed to load colFS.");

    vkDestroyShaderModule(logical_device, triangleVS, nullptr);
    vkDestroyShaderModule(logical_device, triangleFS, nullptr);
    vkDestroyShaderModule(logical_device, screenVS, nullptr);
    vkDestroyShaderModule(logical_device, screenFS, nullptr);
    vkDestroyShaderModule(logical_device, colFS, nullptr);

  }
  
VkShaderModule loadShader(const char* path)
{
	FILE* file = fopen(path, "rb");
	ASSERT(file, path);

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	ASSERT(length >= 0, "Empty File.");
	fseek(file, 0, SEEK_SET);

	char* buffer[length];
	ASSERT(buffer,"");

	size_t rc = fread(buffer, 1, length, file);
	ASSERT(rc == size_t(length), "Failed to read");
	fclose(file);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = length;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer);

	VkShaderModule shaderModule = 0;
	VK_CHECK(vkCreateShaderModule(logical_device, &createInfo, 0, &shaderModule));

	return shaderModule;
}

} //namespace shaders

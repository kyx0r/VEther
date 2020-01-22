
#include "../glsl_compiler/StandAlone/StandAlone.h"
#include "shaders.h"
#include "zone.h"
#include "render.h"
#include "flog.h"
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
	{"./res/shaders/shader.frag.glsl"},
	{"./res/shaders/shader.vert.glsl"},
	{"./res/shaders/shader.tesc.glsl"},
	{"./res/shaders/shader.tese.glsl"},
	{"./res/shaders/skydome.frag.glsl"},
	{"./res/shaders/skydome.vert.glsl"}
};

static char* shaders[][5] =
{
	{"0", sfilenames[0][0], "-V", "-o", "./res/shaders/triangle.frag.spv"},
	{"1", sfilenames[1][0], "-V", "-o", "./res/shaders/triangle.vert.spv"},
	{"2", sfilenames[2][0], "-V", "-o", "./res/shaders/screen.frag.spv"},
	{"3", sfilenames[3][0], "-V", "-o", "./res/shaders/screen.vert.spv"},
	{"4", sfilenames[4][0], "-V", "-o", "./res/shaders/shader.frag.spv"},
	{"5", sfilenames[5][0], "-V", "-o", "./res/shaders/shader.vert.spv"},
	{"6", sfilenames[6][0], "-V", "-o", "./res/shaders/shader.tesc.spv"},
	{"7", sfilenames[7][0], "-V", "-o", "./res/shaders/shader.tese.spv"},
	{"8", sfilenames[8][0], "-V", "-o", "./res/shaders/skydome.tese.spv"},
	{"9", sfilenames[9][0], "-V", "-o", "./res/shaders/skydome.tese.spv"}
};

class UpdateListener : public FW::FileWatchListener
{
public:
	UpdateListener() {}
	void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action)
	{
		info("Reloading shader file: %s %s, action: %d", &dir[0], &filename[0], action);

		// Reload everything because doing this with single shoot does not work
		// for some weird reason and the code grows in complexity as well.
		// now if there are many shaders, then this does not scale at all in terms of performace.

		VK_CHECK(vkDeviceWaitIdle(logical_device));
		DestroyShaders();
		cur_shader_index = 0;
		render::DestroyPipeLines();
		pipelineCount = 0;
		for(uint8_t i = 0; i<ARRAYSIZE(shaders); ++i)
		{
attempt:
			GLSL_COMPILER_ENTRY(ARRAYSIZE(shaders[i]), shaders[i]);
			if(CompileFailed || LinkFailed)
			{
				p("Waiting for disk reaload ...");
				std::this_thread::sleep_for(std::chrono::seconds(1));
				LinkFailed = false;
				CompileFailed = false;
				goto attempt;
			}
		}
		CreatePipelineCache();
		LoadShaders();
	}
};

FW::FileWatcher* fileWatcher;

//this must be called only when logical device is initialized already.
void CompileShaders()
{
	fileWatcher = new FW::FileWatcher(); //calls Z_Malloc()
	for(uint8_t i = 0; i<ARRAYSIZE(shaders); ++i)
	{
attempt:
		GLSL_COMPILER_ENTRY(ARRAYSIZE(shaders[i]), shaders[i]);
		if(CompileFailed || LinkFailed)
		{
			p("Waiting for disk reaload ...");
			std::this_thread::sleep_for(std::chrono::seconds(1));
			LinkFailed = false;
			CompileFailed = false;
			goto attempt;
		}
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
	VkShaderModule shaderFS = shaders::loadShaderMem(4);
	VkShaderModule shaderVS = shaders::loadShaderMem(5);
	VkShaderModule shaderTCS = shaders::loadShaderMem(6);
	VkShaderModule shaderTES = shaders::loadShaderMem(7);
	VkShaderModule SkyDomeFS = shaders::loadShaderMem(8);
	VkShaderModule SkyDomeVS = shaders::loadShaderMem(9);

	render::CreateGraphicsPipeline(pipelineCache, render::BasicTrianglePipe, 0, triangleVS, triangleFS);
	render::CreateGraphicsPipeline(pipelineCache, render::ScreenPipe, 0, screenVS, screenFS);
	render::CreateTessGraphicsPipeline(pipelineCache, render::Vec4FloatPipe, 0, shaderVS, shaderFS, shaderTCS, shaderTES);
	render::CreateGraphicsPipeline(pipelineCache, render::Vec3FloatPipe, 0, SkyDomeVS, SkyDomeFS);
}

void CreatePipelineCache()
{
	VkPipelineCacheCreateInfo cci;
	memset(&cci, 0, sizeof(cci));
	cci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	VK_CHECK(vkCreatePipelineCache(logical_device, &cci, allocators, &pipelineCache));
}

void DestroyShaders()
{
	for(uint32_t i = 0; i<cur_shader_index; i++)
	{
		vkDestroyShaderModule(logical_device, _shaders[i], allocators);
	}
	vkDestroyPipelineCache(logical_device, pipelineCache, allocators);
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

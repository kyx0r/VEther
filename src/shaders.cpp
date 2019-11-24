#include "shaders.h"
#include "flog.h"

/* {
GVAR: logical_device -> startup.cpp
GVAR: VkShaderModule _shaders[20] -> /standalone/standalone.cpp
GVAR: uint32_t cur_shader_index -> /standalone/standalone.cpp
} */

namespace shaders
{

//this must be called only when logical device is initialized already.
void CompileShaders()
{

	static char* shaders[][5] =
	{
		{"dummy", "./res/shaders/triangle.frag.glsl", "-V", "-o", "./res/shaders/triangle.frag.spv"},
		{"dummy", "./res/shaders/triangle.vert.glsl", "-V", "-o", "./res/shaders/triangle.vert.spv"},
		{"dummy", "./res/shaders/screen.frag.glsl", "-V", "-o", "./res/shaders/screen.frag.spv"},
		{"dummy", "./res/shaders/screen.vert.glsl", "-V", "-o", "./res/shaders/screen.vert.spv"},
		{"dummy", "./res/shaders/col.frag.glsl", "-V", "-o", "./res/shaders/col.frag.spv"}

	};
	for(uint32_t i = 0; i<ARRAYSIZE(shaders); ++i)
	{
		GLSL_COMPILER_ENTRY(ARRAYSIZE(shaders[i]), shaders[i]);
	}
	trace("SHADERS COMPILED. \n");
}

VkShaderModule loadShaderMem(int index)
{
	return _shaders[index];
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

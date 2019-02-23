#include "shaders.h"

/* {
GVAR: logical_device -> startup.cpp
} */

namespace shaders
{

void CompileShaders()
{
	/* 	std::vector<char*> shaders;

		shaders.insert(shaders.end(), {"./res/shaders/triangle.vert.glsl", "-V", "-o", "./res/shaders/triangle.vert.spv", "./res/shaders/triangle.vert.glsl"});

		GLSL_COMPILER_ENTRY(shaders.size(), &shaders[0]); */

	static char* shaders[][5] =
	{
		{"./res/shaders/triangle.vert.glsl", "-V", "-o", "./res/shaders/triangle.vert.spv", "./res/shaders/triangle.vert.glsl"},
		{"./res/shaders/triangle.frag.glsl", "-V", "-o", "./res/shaders/triangle.frag.spv", "./res/shaders/triangle.frag.glsl"}
	};
	for(uint32_t i = 0; i<ARRAYSIZE(shaders); i++)
	{
		GLSL_COMPILER_ENTRY(ARRAYSIZE(shaders[i]), shaders[i]);
	}
	//GLSL_COMPILER_ENTRY(ARRAYSIZE(shaders[2]), shaders[1]);

	/* 	static char* argv2[] =
		{"./res/shaders/triangle.frag.glsl", "-V", "-o", "./res/shaders/triangle.frag.spv", "./res/shaders/triangle.frag.glsl"}; */
	//GLSL_COMPILER_ENTRY(ARRAYSIZE(argv), argv2);

	std::cout<<"SHADERS COMPILED. \n";
}

VkShaderModule loadShader(const char* path)
{
	FILE* file = fopen(path, "rb");
	assert(file);

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	assert(length >= 0);
	fseek(file, 0, SEEK_SET);

	char* buffer[length];
	assert(buffer);

	size_t rc = fread(buffer, 1, length, file);
	assert(rc == size_t(length));
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
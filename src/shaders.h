#include "startup.h"
#include "../glsl_compiler/StandAlone/StandAlone.h"

namespace shaders
{

void CompileShaders();
VkShaderModule loadShader(const char* path);

} //namespace shaders
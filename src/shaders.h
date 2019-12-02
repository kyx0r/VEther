#include "startup.h"
#include "../glsl_compiler/StandAlone/StandAlone.h"
#include "../glsl_compiler/SimpleFileWatcher/FileWatcher.h"

namespace shaders
{  
  extern FW::FileWatcher* fileWatcher;
  
void CompileShaders();
VkShaderModule loadShaderMem(int index);
void LoadShaders();
VkShaderModule loadShader(const char* path);
void DestroyShaders();

} //namespace shaders

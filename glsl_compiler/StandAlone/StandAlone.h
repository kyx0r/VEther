// this only applies to the standalone wrapper, not the front end in general
#ifndef _STANDALONE_H
#define _STANDALONE_H

#include "../../src/startup.h"

//imports Vulkan Functions and logical device
#include "../../src/zone.h"

//all shader modules will be stored here.
extern VkShaderModule _shaders[20];
extern uint32_t cur_shader_index;
extern bool CompileFailed;
extern bool LinkFailed;

int GLSL_COMPILER_ENTRY(int argc, char* argv[]);

#endif

// this only applies to the standalone wrapper, not the front end in general
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "../../src/startup.h"
//imports Vulkan Functions and logical device

#include "ResourceLimits.h"
#include "Worklist.h"
#include "DirStackFileIncluder.h"
#include "./../glslang/Include/ShHandle.h"
#include "./../glslang/Include/revision.h"
#include "./../glslang/Public/ShaderLang.h"
#include "../SPIRV/GlslangToSpv.h"
#include "../SPIRV/GLSL.std.450.h"
#include "../SPIRV/doc.h"
#include "../SPIRV/disassemble.h"

#include <cstring>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <array>
#include <map>
#include <memory>
#include <thread>

#include "../glslang/OSDependent/osinclude.h"

#include "../../src/zone.h"

//all shader modules will be stored here.
extern VkShaderModule _shaders[20];
extern uint32_t cur_shader_index;

int C_DECL GLSL_COMPILER_ENTRY(int argc, char* argv[]);

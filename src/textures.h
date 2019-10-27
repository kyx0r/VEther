#include "startup.h"
#include "lodepng.h"

extern VkDescriptorSet	tex_descriptor_sets[2];

namespace textures
{

bool SampleTexture();
void GenerateColorPalette();
void FsLoadPngTexture(const char* filename);
void InitSamplers();
void TexDeinit();
}

#include "startup.h"
#include "lodepng.h"

extern VkDescriptorSet	tex_descriptor_sets[20];

namespace textures
{

bool SampleTexture();
void GenerateColorPalette();
void FsLoadPngTexture(const char* filename);
void InitSamplers();
void TexDeinit();
 void UpdateTexture(unsigned char* image, int w, int h, int index);
 bool SampleTextureUpdate();
}

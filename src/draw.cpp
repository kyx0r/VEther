#include "draw.h"
#include "control.h"
#include "textures.h"
#include "atlas.h"
#include "flog.h"
/* {
GVAR: logical_device -> startup.cpp
GVAR: command_buffer -> control.cpp
GVAR: pipeline_layout -> render.cpp
GVAR: pipelines -> render.cpp
GVAR: y_wheel = window.cpp;
GVAR: time1 -> window.cpp
} */

namespace draw
{

  void DrawQuad(size_t size, Uivertex* vertices, size_t index_count, uint16_t* index_array)
{
	static VkBuffer buffer[2];
	static VkDeviceSize buffer_offset[2];
	static bool once = true;
	static unsigned char* data;
	static uint16_t* index_data;	
	if(once)
	  {
	    data = control::VertexBufferDigress(size, &buffer[0], &buffer_offset[0]);
	    index_data = (uint16_t*) control::IndexBufferDigress(index_count * sizeof(uint16_t), &buffer[1], &buffer_offset[1]);
	    once = false;
	  }

        zone::Q_memcpy(data, &vertices[0], size);
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer[0], &buffer_offset[0]);

	zone::Q_memcpy(index_data, index_array, index_count * sizeof(uint16_t));
	vkCmdBindIndexBuffer(command_buffer, buffer[1], buffer_offset[1], VK_INDEX_TYPE_UINT16);
	
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[1]);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1, 1, &tex_descriptor_sets[0], 0, nullptr);
        vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);
}

void DrawIndexedTriangle(size_t size, Vertex_* vertices, size_t index_count, uint32_t* index_array)
{
	//printf("goto \n");
	static VkBuffer buffer[3];
	static VkDeviceSize buffer_offset[2];
	static VkDescriptorSet dset;
	static uint32_t uniform_offset;
	static bool once = true;
	static unsigned char* data;
	static uint32_t* index_data;
	static	UniformMatrix* mat;
	if(once)
	{
		data = control::VertexBufferDigress(size, &buffer[0], &buffer_offset[0]);
		index_data = (uint32_t*) control::IndexBufferDigress(index_count * sizeof(uint32_t), &buffer[1], &buffer_offset[1]);
		mat = (UniformMatrix*) control::UniformBufferDigress(sizeof(UniformMatrix), &buffer[2], &uniform_offset, &dset);
		once = false;
	}

	zone::Q_memcpy(data, &vertices[0], size);
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer[0], &buffer_offset[0]);

	zone::Q_memcpy(index_data, index_array, index_count * sizeof(uint32_t));
	vkCmdBindIndexBuffer(command_buffer, buffer[1], buffer_offset[1], VK_INDEX_TYPE_UINT32);

	//	ScaleMatrix(mat->model, y_wheel/10, y_wheel/10, 0.0f);
	// float radius = 0.5f;

	// vec3_t eye = {(float)sin(time1) * radius, 0.0f, (float)cos(time1) * radius};
	// vec3_t origin = {0.0f, 0.0f, 0.0f};
	// vec3_t up = {0.0f, 1.0f, 0.0};

	// LookAt(mat->model, eye, origin, up);
	//PrintMatrix(mat->proj);
	//TranslationMatrix(mat->model, (float)xm_norm, (float)ym_norm, 0.0f);

	RotationMatrix(mat->model, DEG2RAD(0), 0.0f, 0.0f, 1.0f);
	RotationMatrix(mat->proj, DEG2RAD(0), 1.0f, 0.0f, 0.0f);
	RotationMatrix(mat->view, DEG2RAD(0), 0.0f, 1.0f, 0.0f);
	//PrintMatrix(mat->view);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0]);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &dset, 1, &uniform_offset);
	//vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1, 1, &tex_descriptor_sets[0], 0, nullptr);
	vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);
}

int r_get_text_width(const char *text, int len)
{
  int res = 0;
  for (const char *p = text; *p && len--; p++) {
    if ((*p & 0xc0) == 0x80) { continue; }
    int chr = mu_min((unsigned char) *p, 127);
    res += atlas[6 + chr].w;
  }
  return res;
}

int text_width(mu_Font font, const char *text, int len)
{
  if (len == -1) { len = strlen(text); }
  return r_get_text_width(text, len);
}

int text_height(mu_Font font)
{
  return 18;
}

int buf_idx;
#define BUFFER_SIZE 1000
Uivertex vert[BUFFER_SIZE * 4];
uint32_t index_buf[BUFFER_SIZE * 6];
  
static void push_quad(mu_Rect dst, mu_Rect src, mu_Color color)
{
  buf_idx--;
  int texvert_idx = buf_idx * 4;
  int   index_idx = buf_idx * 6;  
  ASSERT(buf_idx < BUFFER_SIZE, "Out of ui memory!");
  //  p("%d", buf_idx);
  ASSERT(buf_idx > 0, "buf_idx < 0");
  
  vert[texvert_idx + 0].pos[0] = dst.x;
  vert[texvert_idx + 0].pos[1] = dst.y;
    
  vert[texvert_idx + 1].pos[0] = dst.x + dst.w;
  vert[texvert_idx + 1].pos[1] = dst.y;
    
  vert[texvert_idx + 2].pos[0] = dst.x;
  vert[texvert_idx + 2].pos[1] = dst.y + dst.h;
    
  vert[texvert_idx + 3].pos[0] = dst.x + dst.w;
  vert[texvert_idx + 3].pos[1] = dst.y + dst.h;
  
  /* update texture buffer */
  float x = src.x / (float) ATLAS_WIDTH;
  float y = src.y / (float) ATLAS_HEIGHT;
  float w = src.w / (float) ATLAS_WIDTH;
  float h = src.h / (float) ATLAS_HEIGHT;
  vert[texvert_idx + 0].tex_coord[0] = x;
  vert[texvert_idx + 0].tex_coord[1] = y;
  vert[texvert_idx + 1].tex_coord[0] = x + w;
  vert[texvert_idx + 1].tex_coord[1] = y;
  vert[texvert_idx + 2].tex_coord[0] = x;
  vert[texvert_idx + 2].tex_coord[1] = y + h;
  vert[texvert_idx + 3].tex_coord[0] = x + w;
  vert[texvert_idx + 3].tex_coord[1] = y + h;

  memcpy(&vert[texvert_idx + 0].color, &color, 4);
  memcpy(&vert[texvert_idx + 1].color, &color, 4);
  memcpy(&vert[texvert_idx + 2].color, &color, 4);
  memcpy(&vert[texvert_idx + 3].color, &color, 4);
  
  /* update index buffer */
  index_buf[index_idx + 0] = texvert_idx + 0;
  index_buf[index_idx + 1] = texvert_idx + 1;
  index_buf[index_idx + 2] = texvert_idx + 2;
  index_buf[index_idx + 3] = texvert_idx + 2;
  index_buf[index_idx + 4] = texvert_idx + 3;
  index_buf[index_idx + 5] = texvert_idx + 1;
  
}

void PresentUI()
{
	static VkBuffer buffer[2];
	static VkDeviceSize buffer_offset[2];
	static bool once = true;
	static unsigned char* data;
	static uint32_t* index_data;	
	if(once)
	  {
	    data = control::VertexBufferDigress(sizeof(Uivertex) * BUFFER_SIZE * 8, &buffer[0], &buffer_offset[0]);
	    index_data = (uint32_t*) control::IndexBufferDigress(BUFFER_SIZE * 6 * sizeof(uint32_t), &buffer[1], &buffer_offset[1]);
	    once = false;
	  }

        zone::Q_memcpy(data, &vert[0], sizeof(Uivertex) * 4 * buf_idx);
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer[0], &buffer_offset[0]);

	zone::Q_memcpy(index_data, index_buf, buf_idx * 6 * sizeof(uint32_t));
	vkCmdBindIndexBuffer(command_buffer, buffer[1], buffer_offset[1], VK_INDEX_TYPE_UINT32);
	
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[1]);
        vkCmdDrawIndexed(command_buffer, buf_idx * 6, 1, 0, 0, 0);
}

void InitAtlasTexture()
{
	int mark = zone::Hunk_LowMark();
        unsigned char* out = textures::Tex8to32(atlas_texture, ATLAS_WIDTH * ATLAS_HEIGHT);
	textures::UploadTexture(out, ATLAS_WIDTH, ATLAS_HEIGHT);
	zone::Hunk_FreeToLowMark(mark);  
}

void r_draw_text(const char *text, mu_Vec2 pos, mu_Color color) {
  mu_Rect dst = { pos.x, pos.y, 0, 0 };
  for (const char *p = text; *p; p++)
  {
    if ((*p & 0xc0) == 0x80) { continue; }
    int chr = mu_min((unsigned char) *p, 127);
    //p("%d",chr+6-32);
    mu_Rect src = atlas[chr+6-32];
    dst.w = src.w;
    dst.h = src.h;
    //    push_quad(dst, src, color);
    dst.x += dst.w;
  }
}

void r_draw_rect(mu_Rect rect, mu_Color color)
{
  push_quad(rect, atlas[5], color);
}
  


}

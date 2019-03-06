#include "glm/glm.hpp"

/* {
GVAR: logical_device -> startup.cpp
GVAR: command_buffer -> control.cpp
GVAR: pipeline_layout -> render.cpp
GVAR: pipelines -> render.cpp
} */

typedef struct
{
	float	position[3];
	float	texcoord[2];
	unsigned char	color[4];
} basicvertex_t;

//This file contains drawing functions.
namespace draw
{

inline void Draw_Triangle(size_t size, Vertex *vertices)
{
	VkBuffer buffer;
	VkDeviceSize buffer_offset;
	unsigned char* data = control::VertexAllocate(size, &buffer, &buffer_offset);
	memcpy(data, &vertices[0], size);
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0]);
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer, &buffer_offset);
	vkCmdDraw(command_buffer, sizeof(vertices[0]), 1, 0, 0);
}

inline void Draw_FillCharacterQuad(int x, int y, char num, basicvertex_t *output)
{
	int				row, col;
	float			frow, fcol, size;

	row = num>>4;
	col = num&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	basicvertex_t corner_verts[4];
	memset(&corner_verts, 255, sizeof(corner_verts));

	corner_verts[0].position[0] = x;
	corner_verts[0].position[1] = y;
	corner_verts[0].position[2] = 0.0f;
	corner_verts[0].texcoord[0] = fcol;
	corner_verts[0].texcoord[1] = frow;

	corner_verts[1].position[0] = x+8;
	corner_verts[1].position[1] = y;
	corner_verts[1].position[2] = 0.0f;
	corner_verts[1].texcoord[0] = fcol + size;
	corner_verts[1].texcoord[1] = frow;

	corner_verts[2].position[0] = x+8;
	corner_verts[2].position[1] = y+8;
	corner_verts[2].position[2] = 0.0f;
	corner_verts[2].texcoord[0] = fcol + size;
	corner_verts[2].texcoord[1] = frow + size;

	corner_verts[3].position[0] = x;
	corner_verts[3].position[1] = y+8;
	corner_verts[3].position[2] = 0.0f;
	corner_verts[3].texcoord[0] = fcol;
	corner_verts[3].texcoord[1] = frow + size;

	output[0] = corner_verts[0];
	output[1] = corner_verts[1];
	output[2] = corner_verts[2];
	output[3] = corner_verts[2];
	output[4] = corner_verts[3];
	output[5] = corner_verts[0];
}

inline void Draw_Character(int x, int y, int num)
{
	if (y <= -8)
		return;			// totally off screen

	num &= 255;

	if (num == 32)
		return; //don't waste verts on spaces

	/* 	VkBuffer buffer;
		VkDeviceSize buffer_offset;
		basicvertex_t* vertices = (basicvertex_t*) control::VertexAllocate(6 * sizeof(basicvertex_t), &buffer, &buffer_offset);

		Draw_FillCharacterQuad(x, y, (char)num, vertices);

		vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer, &buffer_offset); */
	//I am jumping ahead of myself here. It looks like extensive texture loading is required before I can do this .
	//BindPipeline(pipelines[1]);
	//vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &char_texture->descriptor_set, 0, NULL);
	//vkCmdDraw(command_buffer, 6, 1, 0, 0);
}

} //namespace draw
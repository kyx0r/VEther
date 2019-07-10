#include "draw.h"
#include "control.h"
#include "textures.h"

/* {
GVAR: logical_device -> startup.cpp
GVAR: command_buffer -> control.cpp
GVAR: pipeline_layout -> render.cpp
GVAR: pipelines -> render.cpp
} */

namespace draw
{

void DrawTriangle(size_t size, Vertex_* vertices)
{
	VkBuffer buffer;
	VkDeviceSize buffer_offset;
	VkDescriptorSet dset;
	uint32_t uniform_offset;

	unsigned char* data = control::VertexBufferDigress(size, &buffer, &buffer_offset);
	memcpy(data, &vertices[0], size);
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer, &buffer_offset);

	UniformMatrix* mat = (UniformMatrix*) control::UniformBufferDigress(sizeof(UniformMatrix), &buffer, &uniform_offset, &dset);

	float m[16];
	static float rot = 0.0f;

	//rot += 0.9f;
	RotationMatrix(m, DEG2RAD(rot), 0.0f, 0.0f, 0.0f);
	memcpy(mat->model, m, 16 * sizeof(float));
	ScaleMatrix(m, 0.8f, 0.8f, 0.8f);
	memcpy(mat->proj, m, 16 * sizeof(float));
	memcpy(mat->view, m, 16 * sizeof(float));

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0]);
	control::BindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, 1, &uniform_offset);
	vkCmdDraw(command_buffer, sizeof(vertices[0]), 1, 0, 0);
}

void DrawIndexedTriangle(size_t size, Vertex_* vertices, size_t index_count, uint16_t* index_array)
{
	//printf("goto \n");
	VkBuffer buffer;
	VkDeviceSize buffer_offset;
	VkDescriptorSet dset;
	uint32_t uniform_offset;

	unsigned char* data = control::VertexBufferDigress(size, &buffer, &buffer_offset);
	memcpy(data, &vertices[0], size);
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer, &buffer_offset);

	uint16_t* index_data = (uint16_t*) control::IndexBufferDigress(index_count * sizeof(uint16_t), &buffer, &buffer_offset);
	memcpy(index_data, index_array, index_count * sizeof(uint16_t));
	vkCmdBindIndexBuffer(command_buffer, buffer, buffer_offset, VK_INDEX_TYPE_UINT16);

	UniformMatrix* mat = (UniformMatrix*) control::UniformBufferDigress(sizeof(UniformMatrix), &buffer, &uniform_offset, &dset);

	static float rot = 0.0f;
	rot += 0.3f;

	RotationMatrix(mat->model, DEG2RAD(rot), 0.0f, 0.0f, 1.0f);
      	RotationMatrix(mat->proj, DEG2RAD(rot+45.0f), 1.0f, 0.0f, 0.0f);
	RotationMatrix(mat->view, DEG2RAD(-rot), 0.0f, 1.0f, 0.0f);
	
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0]);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &dset, 1, &uniform_offset);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1, 1, &tex_descriptor_sets[0], 0, nullptr);
	vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);
}



}

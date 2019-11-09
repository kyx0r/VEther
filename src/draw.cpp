#include "draw.h"
#include "control.h"
#include "textures.h"

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

  void DrawQuad(size_t size, Vertex_* vertices, size_t index_count, uint16_t* index_array)
{
	VkBuffer buffer;
	VkDeviceSize buffer_offset;
	VkDescriptorSet dset;

	unsigned char* data = control::VertexBufferDigress(size, &buffer, &buffer_offset);
	memcpy(data, &vertices[0], size);
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer, &buffer_offset);

	uint16_t* index_data = (uint16_t*) control::IndexBufferDigress(index_count * sizeof(uint16_t), &buffer, &buffer_offset);
	zone::Q_memcpy(index_data, index_array, index_count * sizeof(uint16_t));
	vkCmdBindIndexBuffer(command_buffer, buffer, buffer_offset, VK_INDEX_TYPE_UINT16);
	
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



}

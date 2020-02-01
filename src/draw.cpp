#include "draw.h"
#include "control.h"
#include "textures.h"
#include "atlas.h"
#include "flog.h"
#include "entity.h"

/* {
GVAR: logical_device -> startup.cpp
GVAR: command_buffer -> control.cpp
GVAR: pipeline_layout -> render.cpp
GVAR: pipelines -> render.cpp
GVAR: y_wheel = window.cpp;
GVAR: time1 -> window.cpp
GVAR: meshes -> entity.cpp
} */

static ui_ent_t ui;
static sky_ent_t sky;

namespace draw
{

void Quad(size_t size, Uivertex* vertices, size_t index_count, uint16_t* index_array)
{
	static VkBuffer buffer[2];
	static VkDeviceSize buffer_offset[2];
	static unsigned char* data = nullptr;
	static uint16_t* index_data;
	if(!data)
	{
		data = control::VertexBufferDigress(size, &buffer[0], &buffer_offset[0]);
		index_data = (uint16_t*) control::IndexBufferDigress(index_count * sizeof(uint16_t), &buffer[1], &buffer_offset[1]);
	}

	zone::Q_memcpy(data, &vertices[0], size);
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer[0], &buffer_offset[0]);

	zone::Q_memcpy(index_data, index_array, index_count * sizeof(uint16_t));
	vkCmdBindIndexBuffer(command_buffer, buffer[1], buffer_offset[1], VK_INDEX_TYPE_UINT16);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[1]);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout[0], 1, 1, &tex_descriptor_sets[0], 0, nullptr);
	vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);
}

void Triangle(size_t size, float4_t* vertices)
{
	static VkBuffer buffer;
	static VkDeviceSize buffer_offset;
	static unsigned char* data = nullptr;
	if(!data)
	{
		data = control::VertexBufferDigress(size, &buffer, &buffer_offset);
	}

	zone::Q_memcpy(data, &vertices[0], size);
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer, &buffer_offset);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[2]);
	vkCmdDraw(command_buffer, 3, 1, 0, 0);
}

void IndexedTriangle(size_t size, Vertex_* vertices, size_t index_count, uint32_t* index_array, uint8_t pidx)
{
	static VkBuffer buffer[4];
	static VkDeviceSize buffer_offset[2];
	static VkDescriptorSet dset[2];
	static uint32_t uniform_offset[2];
	static unsigned char* data = nullptr;
	static uint32_t* index_data;
	static UniformMatrix* mat;
	if(!data)
	{
		data = control::VertexBufferDigress(size, &buffer[0], &buffer_offset[0]);
		index_data = (uint32_t*) control::IndexBufferDigress(index_count * sizeof(uint32_t), &buffer[1], &buffer_offset[1]);
		mat = (UniformMatrix*) control::UniformBufferDigress(sizeof(UniformMatrix), &buffer[2], &uniform_offset[0], &dset[0], 0);
	}

	zone::Q_memcpy(data, &vertices[0], size);
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer[0], &buffer_offset[0]);

	zone::Q_memcpy(index_data, index_array, index_count * sizeof(uint32_t));
	vkCmdBindIndexBuffer(command_buffer, buffer[1], buffer_offset[1], VK_INDEX_TYPE_UINT32);

	IdentityMatrix(mat->proj);
	IdentityMatrix(mat->model);
	IdentityMatrix(mat->view);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[pidx]);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout[0], 0, 1, &dset[0], 1, &uniform_offset[0]);
	//vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout[0], 1, 1, &dset[1], 1, &uniform_offset[1]);
	//vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout[0], 2, 1, &tex_descriptor_sets[0], 0, nullptr);
	vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);
}

void Meshes()
{
	mesh_ent_t* head = meshes;
	while(head->vertex_data != nullptr)
	{
		btTransform transform; //update position
		head->rigidBody->getMotionState()->getWorldTransform(transform);
		btVector3 origin = transform.getOrigin();
		TranslationMatrix(head->mat->model, origin.getX(), origin.getY(), origin.getZ());

		vkCmdBindVertexBuffers(command_buffer, 0, 1, &head->buffer[0], &head->buffer_offset[0]);
		vkCmdBindIndexBuffer(command_buffer, head->buffer[1], head->buffer_offset[1], VK_INDEX_TYPE_UINT32);
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0]);
		vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout[0], 0, 1, &head->dset[0], 1, &head->uniform_offset[0]);
		vkCmdDrawIndexed(command_buffer, head->index_count, 1, 0, 0, 0);
		head = head->next;
	}
}

void Line(vec3_t from, vec3_t to, vec3_t color)
{
	Vertex_ v[2];
	v[0].pos[0] = from[0];
	v[0].pos[1] = from[1];
	v[0].pos[2] = from[2];
	v[1].pos[0] = to[0];
	v[1].pos[1] = to[1];
	v[1].pos[2] = to[2];
	v[0].color[0] = color[0];
	v[0].color[1] = color[1];
	v[0].color[2] = color[2];
	v[1].color[0] = color[0];
	v[1].color[1] = color[1];
	v[1].color[2] = color[2];
	uint32_t ib[2];
	ib[0] = 0;
	ib[1] = 1;
	IndexedTriangle(sizeof(v), &v[0], ARRAYSIZE(ib), &ib[0], 4);
}

int r_get_text_width(const char *text, int len)
{
	int res = 0;
	for (const char *p = text; *p && len--; p++)
	{
		if ((*p & 0xc0) == 0x80)
		{
			continue;
		}
		int chr = mu_min((unsigned char) *p, 127);
		res += atlas[chr-27].w;
	}
	return res;
}

int text_width(mu_Font font, const char *text, int len)
{
	if (len == -1)
	{
		len = strlen(text);
	}
	return r_get_text_width(text, len);
}

int text_height(mu_Font font)
{
	return 18;
}

static void push_quad(mu_Rect dst, mu_Rect src, mu_Color color, bool tex)
{
	int texvert_idx = ui.buf_idx * 4;
	int   index_idx = ui.buf_idx * 6;
	ui.buf_idx++;
	if(ui.buf_idx == ui.buffer_size-1)
	{
		warn("Possibly out of ui memory!");
		InitUI();
	}
	Uivertex* vert = (Uivertex*) ui.vertex_data;
	uint32_t* index_buf = ui.index_data;

	vert[texvert_idx + 0].pos[0] = dst.x;
	vert[texvert_idx + 0].pos[1] = dst.y;

	vert[texvert_idx + 1].pos[0] = dst.x + dst.w;
	vert[texvert_idx + 1].pos[1] = dst.y;

	vert[texvert_idx + 2].pos[0] = dst.x;
	vert[texvert_idx + 2].pos[1] = dst.y + dst.h;

	vert[texvert_idx + 3].pos[0] = dst.x + dst.w;
	vert[texvert_idx + 3].pos[1] = dst.y + dst.h;

	if(tex)
	{
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
	}
	else
	{
		vert[texvert_idx + 0].tex_coord[0] = FLT_MAX;
		vert[texvert_idx + 1].tex_coord[0] = FLT_MAX;
		vert[texvert_idx + 2].tex_coord[0] = FLT_MAX;
		vert[texvert_idx + 3].tex_coord[0] = FLT_MAX;
	}
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
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &ui.buffer[0], &ui.buffer_offset[0]);
	vkCmdBindIndexBuffer(command_buffer, ui.buffer[1], ui.buffer_offset[1], VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[1]);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout[0], 2, 1, &tex_descriptor_sets[0], 0, nullptr);
	vkCmdDrawIndexed(command_buffer, ui.buf_idx * 6, 1, 0, 0, 0);
	ui.buf_idx = 0;
}

void InitUI()
{
	ui.buffer_size += 1000;
	ui.vertex_data = control::VertexBufferDigress(sizeof(Uivertex) * ui.buffer_size * 4, &ui.buffer[0], &ui.buffer_offset[0]);
	ui.index_data = (uint32_t*) control::IndexBufferDigress(ui.buffer_size * 6 * sizeof(uint32_t), &ui.buffer[1], &ui.buffer_offset[1]);
}

void InitAtlasTexture()
{
	textures::UploadTexture(atlas_texture, ATLAS_WIDTH, ATLAS_HEIGHT, VK_FORMAT_R8_UNORM);
}

void Text(const char *text, mu_Vec2 pos, mu_Color color)
{
	mu_Rect dst = { pos.x, pos.y, 0, 0 };
	for (const char *p = text; *p; p++)
	{
		if ((*p & 0xc0) == 0x80)
		{
			continue;
		}
		int chr = mu_min((unsigned char) *p, 127);
		mu_Rect src = atlas[chr-27];
		dst.w = src.w;
		dst.h = src.h;
		push_quad(dst, src, color, true);
		dst.x += dst.w;
	}
}

void Rect(mu_Rect rect, mu_Color color)
{
	push_quad(rect, atlas[5], color, false);
}


void Icon(int id, mu_Rect rect, mu_Color color)
{
	mu_Rect src = atlas[id-1];
	int x = rect.x + (rect.w - src.w) / 2;
	int y = rect.y + (rect.h - src.h) / 2;
	push_quad(mu_rect(x, y, src.w, src.h), src, color, true);
}

void Stats()
{
	char output[75] = "Frametime:            ";
	snprintf(&output[12], 50, "%f", frametime);
	Text(output, {0,0}, {255, 0, 0, 255});
	zone::Q_memcpy(output, "FPS:            ", 16);
	snprintf(&output[5], 50, "%f", lastfps);
	Text(output, {0,10}, {255, 0, 0, 255});
}


void Cursor()
{
	static int alpha = 255;
	static char direction = 0;

	if(xm > 0 && ym > 0)
	{
		(direction) ? alpha-- : alpha++;
		if(alpha > 254)
		{
			direction = 1;
		}
		if (alpha < 50)
		{
			direction = 0;
		}
		for(int i = 0; i<2; i++)
		{

			int texvert_idx = ui.buf_idx * 4;
			int   index_idx = ui.buf_idx * 6;
			ui.buf_idx++;
			if(ui.buf_idx == ui.buffer_size-1)
			{
				warn("Possibly out of ui memory!");
				InitUI();
			}
			Uivertex* vert = (Uivertex*) ui.vertex_data;
			uint32_t* index_buf = ui.index_data;

			vert[texvert_idx + 0].pos[0] = xm;
			vert[texvert_idx + 0].pos[1] = ym;
			vert[texvert_idx + 1].pos[0] = xm+10;
			vert[texvert_idx + 1].pos[1] = ym+10;
			if(!i)
			{
				vert[texvert_idx + 2].pos[0] = xm+10;
				vert[texvert_idx + 2].pos[1] = ym+25;
			}
			else
			{
				//draw the second half of the cursor
				vert[texvert_idx + 2].pos[0] = xm+25;
				vert[texvert_idx + 2].pos[1] = ym+10;
			}

			vert[texvert_idx + 3].pos[0] = vert[texvert_idx + 2].pos[0];
			vert[texvert_idx + 3].pos[1] = vert[texvert_idx + 2].pos[1];
			vert[texvert_idx + 0].tex_coord[0] = FLT_MAX;
			vert[texvert_idx + 1].tex_coord[0] = FLT_MAX;
			vert[texvert_idx + 2].tex_coord[0] = FLT_MAX;
			vert[texvert_idx + 3].tex_coord[0] = FLT_MAX;

			mu_Color color;
			color.r = 255;
			color.g = 69;
			color.b = 0;
			color.a = alpha;
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
	}
}

void InitSkydome()
{
	// Generate sphere
	float radius = 10.0f;
	unsigned int slices = 25;
	unsigned int stacks = 25;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Vertex vertex;
	vertex.x = 0.0f;
	vertex.y = radius;
	vertex.z = 0.0f;
	vertices.push_back(vertex);

	float phiStep = M_PI / stacks;
	float thetaStep = 2.0f * M_PI / slices;

	for (unsigned int i = 1; i <= stacks - 1; i++)
	{
		float phi = i * phiStep;
		for (unsigned int j = 0; j <= slices; j++)
		{
			float theta = j * thetaStep;
			vertex.x = radius * sin(phi) * cos(theta);
			vertex.y = radius * cos(phi);
			vertex.z = radius * sin(phi) * sin(theta);
			vertices.push_back(vertex);
		}
	}

	vertex.x = 0.0f;
	vertex.y = -radius;
	vertex.z = 0.0f;
	vertices.push_back(vertex);

	for (unsigned int i = 1; i <= slices; i++)
	{
		indices.push_back(0);
		indices.push_back(i + 1);
		indices.push_back(i);
	}

	int baseIndex = 1;
	int ringVertexCount = slices + 1;
	for (unsigned int i = 0; i < stacks - 2; i++)
	{
		for (unsigned int j = 0; j < slices; j++)
		{
			indices.push_back(baseIndex + i * ringVertexCount + j);
			indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

			indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
			indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
		}
	}

	int southPoleIndex = (int)vertices.size() - 1;
	baseIndex = southPoleIndex - ringVertexCount;
	for (unsigned int i = 0; i < slices; i++)
	{
		indices.push_back(southPoleIndex);
		indices.push_back(baseIndex + i);
		indices.push_back(baseIndex + i + 1);
	}

	sky.n_vertices = (unsigned int)vertices.size();
	sky.n_indices = (unsigned int)indices.size();
	sky.vertex_data = control::VertexBufferDigress(sizeof(Vertex) *  sky.n_vertices, &sky.buffer[0], &sky.buffer_offset[0]);
	sky.index_data = (uint32_t*) control::IndexBufferDigress(sizeof(uint32_t) * sky.n_indices, &sky.buffer[1], &sky.buffer_offset[1]);
	sky.sky_uniform = (UniformSkydome*) control::UniformBufferDigress(sizeof(UniformSkydome), &sky.buffer[2], &sky.uniform_offset[0], &sky.dset[0], 1);
	sky.tmat = (Matrix*) control::UniformBufferDigress(sizeof(Matrix), &sky.buffer[3], &sky.uniform_offset[1], &sky.dset[1], 0);
	zone::Q_memcpy(sky.vertex_data, vertices.data(), sizeof(Vertex) * sky.n_vertices);
	zone::Q_memcpy(sky.index_data, indices.data(), sizeof(uint32_t) * sky.n_indices);
}

void SkyDome()
{
	sky.sky_uniform->SkyColor[0] = 0.33f;
	sky.sky_uniform->SkyColor[1] = 0.66f;
	sky.sky_uniform->SkyColor[2] = 0.99f;
	sky.sky_uniform->SkyColor[3] = 1.0f;
	sky.sky_uniform->almoshereColor[0] = 1.0f;
	sky.sky_uniform->almoshereColor[1] = 0.4f;
	sky.sky_uniform->almoshereColor[2] = 1.0f;
	sky.sky_uniform->almoshereColor[3] = 1.0f;
	sky.sky_uniform->groundColor[0] = 0.5f;
	sky.sky_uniform->groundColor[1] = 0.5f;
	sky.sky_uniform->groundColor[2] = 0.5f;
	sky.sky_uniform->groundColor[3] = 0.5f;
	sky.sky_uniform->atmosphereHeight = 0.25f;

	TranslationMatrix(sky.tmat->mat, cam.pos[0], cam.pos[1], cam.pos[2]);

	vkCmdBindVertexBuffers(command_buffer, 0, 1, &sky.buffer[0], &sky.buffer_offset[0]);
	vkCmdBindIndexBuffer(command_buffer, sky.buffer[1], sky.buffer_offset[1], VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[3]);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout[0], 1, 1, &sky.dset[0], 1, &sky.uniform_offset[0]);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout[0], 0, 1, &sky.dset[1], 1, &sky.uniform_offset[1]);
	vkCmdDrawIndexed(command_buffer, sky.n_indices, 1, 0, 0, 0);
}

}

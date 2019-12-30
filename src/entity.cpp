#include "entity.h"
#include "flog.h"
#include "window.h"
#include "zone.h"
#include "control.h"

cam_ent_t cam;
mesh_ent_t* meshes;

static char* smeshes[][1] =
{
	{"./res/kitty.obj"},
};

namespace entity
{

void ViewMatrix(float matrix[16])
{
	vec3_t euler;
	_VectorAdd(cam.pos, cam.front, euler);
	LookAt(matrix, cam.pos, euler, cam.up);
}

void UpdateCamera()
{
	cam.front[0] = cos(DEG2RAD(cam.yaw)) * cos(DEG2RAD(cam.pitch));
	cam.front[1] = sin(DEG2RAD(cam.pitch));
	cam.front[2] = sin(DEG2RAD(cam.yaw)) * cos(DEG2RAD(cam.pitch));
	VectorNormalize(cam.front);
	CrossProduct(cam.front, cam.worldup, cam.right);
	VectorNormalize(cam.right);
	CrossProduct(cam.right, cam.front, cam.up);
	VectorNormalize(cam.up);
}

void InitCamera()
{
	trace("Initializing camera");

	cam.fovx = AdaptFovx(90.0f, window_width, window_height);
	cam.fovy = CalcFovy(cam.fovx, window_width, window_height);

	cam.pos[0] = 0.0f;
	cam.pos[1] = 0.0f;
	cam.pos[2] = -3.0f;
	cam.front[0] = 0.0f;
	cam.front[1] = 0.0f;
	cam.front[2] = -1.0f;
	cam.worldup[0] = 0.0f;
	cam.worldup[1] = 1.0f;
	cam.worldup[2] = 0.0f;
	cam.yaw = 90.0f;
	cam.pitch = 0.0f;
	cam.speed = 2.5f;
	cam.sensitivity = 0.1f;
	cam.zoom = 45.0f;
}

void InitMeshes()
{
	meshes = (mesh_ent_t*) zone::Z_Malloc(sizeof(mesh_ent_t));
	mesh_ent_t* head = meshes;
	for(uint8_t i = 0; i<ARRAYSIZE(smeshes); i++)
	{
		head->id = i; 
		head->obj = LoadOBJ(smeshes[i][0]);
		head->vertex_data = control::VertexBufferDigress(head->obj.renderables->vertex_count * sizeof(Vertex_), &head->buffer[0], &head->buffer_offset[0]);
		head->index_data = (uint32_t*) control::IndexBufferDigress(
				head->obj.renderables->index_count * sizeof(uint32_t), &head->buffer[1], &head->buffer_offset[1]);
		head->mat = (UniformMatrix*) control::UniformBufferDigress(sizeof(UniformMatrix), &head->buffer[3], &head->uniform_offset[0], &head->dset[0], 0);

		zone::Q_memcpy(head->vertex_data, head->obj.renderables->vertices, head->obj.renderables->vertex_count * sizeof(Vertex_));
		zone::Q_memcpy(head->index_data, head->obj.renderables->indices, head->obj.renderables->index_count * sizeof(uint32_t));
		IdentityMatrix(head->mat->proj);
		IdentityMatrix(head->mat->model);
		IdentityMatrix(head->mat->view);
		head->next = (mesh_ent_t*) zone::Z_Malloc(sizeof(mesh_ent_t));
		head->next->prev = head;
		head = head->next;
	}
}

mesh_ent_t* GetMesh(int id, mesh_ent_t** last)
{
	mesh_ent_t* head = meshes;
	mesh_ent_t* copy = nullptr;
	while(head->vertex_data)
	{
		if(head->id == id)
		{
			copy = head;
		}
		head = head->next;
	}
	if(last)
	{
		*last = head;
	}
	return copy;
}

//make a soft dublicate of the entity 
void InstanceMesh(int id)
{
	mesh_ent_t* head;
	mesh_ent_t* copy = GetMesh(id, &head);
	if(copy)
	{
		// always have an empty node allocated at the back.
		head->parent = copy;
		head->id = head->prev->id;
		head->id++;
		head->obj = copy->obj;
		head->vertex_data = copy->vertex_data;
		head->index_data = copy->index_data;
		zone::Q_memcpy(&head->buffer[0], &copy->buffer[0], sizeof(head->buffer));
		zone::Q_memcpy(&head->dset[0], &copy->dset[0], sizeof(head->dset));
		zone::Q_memcpy(&head->buffer_offset[0], &copy->buffer_offset[0], sizeof(head->buffer_offset));
		head->mat = (UniformMatrix*) control::UniformBufferDigress(sizeof(UniformMatrix), &head->buffer[3], &head->uniform_offset[0], &head->dset[0], 0);
		IdentityMatrix(head->mat->proj);
		IdentityMatrix(head->mat->model);
		IdentityMatrix(head->mat->view);
		head->next = (mesh_ent_t*) zone::Z_Malloc(sizeof(mesh_ent_t));
		head->next->prev = head;
	}
	else
	{
		error("mesh ent id %d not found!", id);
	}

}

void MoveTo(int id, vec3_t pos)
{
	mesh_ent_t* copy = GetMesh(id, nullptr);
	if(copy)
	{
		TranslationMatrix(copy->mat->model, pos[0], pos[1], pos[2]);
	}
	else
	{

		error("mesh ent id %d not found!", id);
	}
}

} //namespace entity

#ifndef __ENTITY_H__
#define __ENTITY_H__

#include "startup.h"
#include "mathlib.h"
#include "render.h"
#include "fast_obj.h"

#include <meshoptimizer.h>

typedef struct cam_ent_t
{
	vec3_t pos;
	vec3_t front;
	vec3_t up;
	vec3_t right;
	vec3_t worldup;
	float proj[16];
	float view[16];
	float mvp[16];

	float yaw;
	float pitch;
	float roll;
	float speed;
	float sensitivity;
	float zoom;
	float fovx;
	float fovy;

} cam_ent_t;
extern cam_ent_t cam;

typedef struct ui_ent_t
{
	VkBuffer buffer[2];
	VkDeviceSize buffer_offset[2];
	unsigned char* vertex_data;
	uint32_t* index_data;
	int buffer_size;
	int buf_idx;
} ui_ent_t;

typedef struct basic_ent_t
{
	VkBuffer buffer[4];
	VkDeviceSize buffer_offset[2];
	uint32_t uniform_offset[2];
	VkDescriptorSet dset[2];
	unsigned char* vertex_data;
	uint32_t* index_data;
	UniformMatrix* mat;
	uint32_t vertex_count;
	uint32_t index_count;
	size_t size;
	uint8_t pidx;
} basic_ent_t;

typedef struct sky_ent_t
{
	VkBuffer buffer[4];
	VkDeviceSize buffer_offset[3];
	VkDescriptorSet dset[2];
	uint32_t uniform_offset[2];
	unsigned char* vertex_data;
	uint32_t* index_data;
	uint32_t n_indices;
	uint32_t n_vertices;
	UniformSkydome* sky_uniform;
	Matrix* tmat;
} sky_ent_t;


typedef struct mesh_ent_t
{
	char* name;
	fastObjMesh* obj;
	VkBuffer buffer[4];
	VkDeviceSize buffer_offset[2];
	VkDescriptorSet dset[2];
	uint32_t uniform_offset[2];
	UniformMatrix* mat;
	unsigned char* vertex_data;
	uint32_t* index_data;
	uint32_t vertex_count;
	uint32_t index_count;
	float scale;
	btTriangleMesh* collisionMesh;
	btGImpactMeshShape* collisionShape;
	btCollisionShape* colShape;
	btRigidBody* rigidBody;
	btScalar mass;
	btVector3 inertia;
	struct mesh_ent_t* next;
	struct mesh_ent_t* prev;
	struct mesh_ent_t* parent;
} mesh_ent_t;
extern mesh_ent_t* meshes;

namespace entity
{
void UpdateCamera();
void InitCamera();
void ViewMatrix(float matrix[16]);
void InitMeshes();
mesh_ent_t* GetMesh(char* name, mesh_ent_t** last);
mesh_ent_t* InstanceMesh(char* name);
void MoveTo(char* name, vec3_t pos);
void SetPosition(mesh_ent_t* copy, vec3_t pos);
void InitPhysics();
void StepPhysics();
void SetupWorldPlane(float size);
btVector3 GetRayTo(int x, int y);
 bool PickBody(const btVector3& rayFromWorld, const btVector3& rayToWorld);
 bool MovePickedBody(const btVector3& rayFromWorld, const btVector3& rayToWorld);
void RemovePickingConstraint();
} //namespace entity
#endif

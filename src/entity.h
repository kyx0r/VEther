#ifndef __ENTITY_H__
#define __ENTITY_H__

#include "startup.h"
#include "mathlib.h"
#include "render.h"
#include "obj_parse.h"

#include <BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#include <btBulletDynamicsCommon.h>

typedef struct cam_ent_t
{
	vec3_t pos;
	vec3_t angles;
	vec3_t front;
	vec3_t up;
	vec3_t right;
	vec3_t worldup;

	float yaw      ;
	float pitch    ;
	float speed    ;
	float sensitivity ;
	float zoom        ;
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
	uint8_t id;
	ParsedOBJ obj;
	VkBuffer buffer[4];
	VkDeviceSize buffer_offset[2];
	VkDescriptorSet dset[2];
	uint32_t uniform_offset[2];
	UniformMatrix* mat;
	unsigned char* vertex_data;
	uint32_t* index_data;
	float scale;
	btTriangleMesh* collisionMesh;
	btGImpactMeshShape* collisionShape;
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
mesh_ent_t* GetMesh(int id, mesh_ent_t** last);
mesh_ent_t* InstanceMesh(int id);
void MoveTo(int id, vec3_t pos);
void SetPosition(mesh_ent_t* copy, vec3_t pos);
void InitPhysics();
void StepPhysics();
} //namespace entity
#endif

#include "entity.h"
#include "window.h"
#include "zone.h"
#include "control.h"
#include "flog.h"

cam_ent_t cam;
mesh_ent_t* meshes;
btDiscreteDynamicsWorld* dynamicsWorld;

static btBroadphaseInterface* broadphase;
static btDefaultCollisionConfiguration* collisionConfiguration;
static btCollisionDispatcher* dispatcher;
static btSequentialImpulseConstraintSolver* solver;

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

void RegisterCollisionModel(btTriangleMesh* collisionMesh)
{
	
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
		head->collisionMesh = new btTriangleMesh(true, false); //32 bit indexes, 3 component
		for(uint64_t c = 0; c<head->obj.renderables->vertex_count / 24; c+=24)
		{
			float* coord = &head->obj.renderables->vertices[c];
			btVector3 v1 = btVector3(coord[c], coord[c+1], coord[c+2]);
			btVector3 v2 = btVector3(coord[c+8], coord[c+9], coord[c+10]);
			btVector3 v3 = btVector3(coord[c+16], coord[c+17], coord[c+18]);
			head->collisionMesh->addTriangle(v1, v2, v3);
		}
		head->collisionShape = new btGImpactMeshShape(head->collisionMesh);
		head->collisionShape->setLocalScaling(btVector3(1, 1, 1));
		head->collisionShape->setMargin(0.0f);
		head->collisionShape->updateBound();
		btTransform transform;
		transform.setIdentity();
		btVector3 inertia = btVector3(0.0f, 0.0f, 0.0f);
		head->collisionShape->calculateLocalInertia(100.f, inertia);
		btDefaultMotionState* motionState = new btDefaultMotionState(transform);
		btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(100.f, motionState, head->collisionShape, inertia);
		head->rigidBody = new btRigidBody(rigidBodyCI);
		dynamicsWorld->addRigidBody(head->rigidBody);
		head->rigidBody->setFriction(1.0f);

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

void InitPhysics()
{
	btAlignedAllocSetCustom((btAllocFunc*) Bt_alloc, (btFreeFunc*) Bt_free);
	broadphase = new btDbvtBroadphase();
	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);

	btGImpactCollisionAlgorithm::registerAlgorithm(dispatcher);

	solver = new btSequentialImpulseConstraintSolver();
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);

	dynamicsWorld->setGravity(btVector3(0, -10, 0));
}

void StepPhysics()
{
	dynamicsWorld->stepSimulation((realtime / (1.0f / frametime)), 0);
}

} //namespace entity

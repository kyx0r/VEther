#include "entity.h"
#include "window.h"
#include "zone.h"
#include "control.h"
#include "flog.h"
#include <vector>

cam_ent_t cam;
mesh_ent_t* meshes;
btDiscreteDynamicsWorld* dynamicsWorld;

static btBroadphaseInterface* broadphase;
static btDefaultCollisionConfiguration* collisionConfiguration;
static btCollisionDispatcher* dispatcher;
static btSequentialImpulseConstraintSolver* solver;
static btTypedConstraint* m_pickedConstraint;
static int m_savedState;
static btRigidBody* m_pickedBody;
static btVector3 m_oldPickingPos;
static btVector3 m_hitPos;
static btScalar m_oldPickingDist;

static char* smeshes[][1] =
{
	{"./res/kitty.obj"},
	{"./res/doch.obj"},
	{"./res/plane.obj"},
	{"./res/cube.obj"},
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
	cam.pos[2] = 0.0f;
	cam.front[0] = 0.0f;
	cam.front[1] = 0.0f;
	cam.front[2] = -1.0f;
	cam.worldup[0] = 0.0f;
	cam.worldup[1] = 1.0f;
	cam.worldup[2] = 0.0f;
	cam.yaw = 0.0f;
	cam.pitch = 0.0f;
	cam.speed = 10.f;
	cam.sensitivity = 0.1f;
	cam.zoom = 45.0f;
}

size_t TriangulateObj(fastObjMesh* obj, std::vector<Vertex_>& vertices)
{
	size_t vertex_offset = 0;
	size_t index_offset = 0;
	for (unsigned int i = 0; i < obj->face_count; ++i)
	{
		for (unsigned int j = 0; j < obj->face_vertices[i]; ++j)
		{
			fastObjIndex gi = obj->indices[index_offset + j];
			// triangulate polygon on the fly; offset-3 is always the first polygon vertex
			if (j >= 3)
			{
				vertices[vertex_offset + 0] = vertices[vertex_offset - 3];
				vertices[vertex_offset + 1] = vertices[vertex_offset - 1];
				vertex_offset += 2;
			}

			Vertex_& v = vertices[vertex_offset++];

			v.pos[0] = obj->positions[gi.p * 3 + 0];
			v.pos[1] = obj->positions[gi.p * 3 + 1];
			v.pos[2] = obj->positions[gi.p * 3 + 2];
			if(v.pos[0] > obj->maxvert[0])
			{
				obj->maxvert[0] = v.pos[0];
			}
			if(v.pos[1] > obj->maxvert[1])
			{
				obj->maxvert[1] = v.pos[1];
			}
			if(v.pos[2] > obj->maxvert[2])
			{
				obj->maxvert[2] = v.pos[2];
			}
			v.color[0] = obj->normals[gi.n * 3 + 0];
			v.color[1] = obj->normals[gi.n * 3 + 1];
			v.color[2] = obj->normals[gi.n * 3 + 2];
			v.tex_coord[0] = meshopt_quantizeHalf(obj->texcoords[gi.t * 2 + 0]);
			v.tex_coord[1] = meshopt_quantizeHalf(obj->texcoords[gi.t * 2 + 1]);
		}

		index_offset += obj->face_vertices[i];
	}


	fast_obj_destroy(obj);
	return vertex_offset;
}

void MeshCollisionModel(mesh_ent_t* mesh)
{
	btVector3 inertia = btVector3(0.0f, 0.0f, 0.0f);
	mesh->collisionMesh = new btTriangleMesh();
	mesh->collisionMesh->m_indexedMeshes[0].m_numTriangles = mesh->index_count / 3;
	mesh->collisionMesh->m_indexedMeshes[0].m_numVertices = mesh->vertex_count;
	Vertex_* ptr = (Vertex_*)mesh->vertex_data;
	for(uint32_t i = 0; i<mesh->vertex_count; i++)
	{
		mesh->collisionMesh->m_3componentVertices.push_back(ptr->pos[0]);
		mesh->collisionMesh->m_3componentVertices.push_back(ptr->pos[1]);
		mesh->collisionMesh->m_3componentVertices.push_back(ptr->pos[2]);
		ptr++;
	}

	for(uint32_t i = 0; i<mesh->index_count; i++)
	{
		mesh->collisionMesh->m_32bitIndices.push_back(mesh->index_data[i]);
	}
	mesh->collisionMesh->m_indexedMeshes[0].m_triangleIndexBase = (unsigned char*)&mesh->collisionMesh->m_32bitIndices[0];
	mesh->collisionMesh->m_indexedMeshes[0].m_vertexBase = (unsigned char*)&mesh->collisionMesh->m_3componentVertices[0];

	mesh->collisionShape = new btGImpactMeshShape(mesh->collisionMesh);
	mesh->collisionShape->setLocalScaling(btVector3(1, 1, 1));
	mesh->collisionShape->setMargin(0.0f);
	mesh->collisionShape->updateBound();

	btTransform transform;
	transform.setIdentity();
	btDefaultMotionState* motionState = new btDefaultMotionState(transform);
	btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(0.f, motionState, mesh->collisionShape, inertia);
	mesh->rigidBody = new btRigidBody(rigidBodyCI);
	dynamicsWorld->addRigidBody(mesh->rigidBody);
	mesh->rigidBody->setFriction(1.0f);
}


void SetupWorldPlane(float size)
{
	mesh_ent_t* mesh = GetMesh("./res/plane.obj", nullptr);
	btVector3 inertia = btVector3(0.0f, 0.0f, 0.0f);
	dynamicsWorld->removeRigidBody(mesh->rigidBody);
	delete mesh->colShape;
	delete mesh->rigidBody;
	mesh->colShape = new btBoxShape(btVector3(btScalar(size), btScalar(size), btScalar(size)));
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(0, size, 0));
	btDefaultMotionState* motionState = new btDefaultMotionState(transform);
	btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(0.f, motionState, mesh->colShape, inertia);
	mesh->rigidBody = new btRigidBody(rigidBodyCI);
	dynamicsWorld->addRigidBody(mesh->rigidBody);
	mesh->rigidBody->setFriction(1.0f);
	TranslationMatrix(mesh->mat->view, 0, -size, 0);
	ScaleMatrix(mesh->mat->view, size, size, size);
}

void InitMeshes()
{
	meshes = (mesh_ent_t*) zone::Z_Malloc(sizeof(mesh_ent_t));
	mesh_ent_t* head = meshes;
	for(uint8_t i = 0; i<ARRAYSIZE(smeshes); i++)
	{
		head->name = smeshes[i][0];
		head->obj = fast_obj_read(smeshes[i][0]);
		size_t index_count = 0;
		for (unsigned int i = 0; i < head->obj->face_count; ++i)
		{
			index_count += 3 * (head->obj->face_vertices[i] - 2);
		}

		std::vector<Vertex_> triangle_vertices(index_count);
		size_t offs = TriangulateObj(head->obj, triangle_vertices);
		ASSERT(offs == index_count, "");

		std::vector<uint32_t> remap(index_count);
		size_t vertex_count = meshopt_generateVertexRemap(remap.data(), 0, index_count, triangle_vertices.data(), index_count, sizeof(Vertex_));

		std::vector<Vertex_> vertices(vertex_count);
		std::vector<uint32_t> indices(index_count);

		meshopt_remapVertexBuffer(vertices.data(), triangle_vertices.data(), index_count, sizeof(Vertex_), remap.data());
		meshopt_remapIndexBuffer(indices.data(), 0, index_count, remap.data());

		meshopt_optimizeVertexCache(indices.data(), indices.data(), index_count, vertex_count);
		meshopt_optimizeVertexFetch(vertices.data(), indices.data(), index_count, vertices.data(), vertex_count, sizeof(Vertex_));

		head->mat = (UniformMatrix*) control::UniformBufferDigress(sizeof(UniformMatrix), &head->buffer[3], &head->uniform_offset[0], &head->dset[0], 0);
		head->vertex_data = control::VertexBufferDigress(vertices.size()*sizeof(Vertex_), &head->buffer[0], &head->buffer_offset[0]);
		head->index_data = (uint32_t*) control::IndexBufferDigress(indices.size()*sizeof(uint32_t), &head->buffer[1], &head->buffer_offset[1]);

		head->vertex_count = vertices.size();
		head->index_count = indices.size();
		zone::Q_memcpy(head->vertex_data, vertices.data(), head->vertex_count*sizeof(Vertex_));
		zone::Q_memcpy(head->index_data, indices.data(), head->index_count*sizeof(uint32_t));

		IdentityMatrix(head->mat->proj);
		IdentityMatrix(head->mat->model);
		IdentityMatrix(head->mat->view);
		float approx_bound = (head->obj->maxvert[0] + head->obj->maxvert[1] + head->obj->maxvert[2])/3;
		head->colShape = new btSphereShape(approx_bound);
		btTransform transform;
		transform.setIdentity();
		btVector3 inertia = btVector3(0.0f, 0.0f, 0.0f);
		head->colShape->calculateLocalInertia(100.f, inertia);
		btDefaultMotionState* motionState = new btDefaultMotionState(transform);
		btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(100.f, motionState, head->colShape, inertia);
		head->rigidBody = new btRigidBody(rigidBodyCI);
		dynamicsWorld->addRigidBody(head->rigidBody);
		head->rigidBody->setFriction(1.0f);

		head->next = (mesh_ent_t*) zone::Z_Malloc(sizeof(mesh_ent_t));
		head->next->prev = head;
		head = head->next;
	}
}

mesh_ent_t* GetMesh(char* name, mesh_ent_t** last)
{
	mesh_ent_t* head = meshes;
	mesh_ent_t* copy = nullptr;
	while(head->vertex_data)
	{
		if(head->name)
		{
			if(zone::Q_strcmp(head->name, name) == 0)
			{
				copy = head;
			}
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
mesh_ent_t* InstanceMesh(char* name)
{
	mesh_ent_t* head = nullptr;
	mesh_ent_t* copy = GetMesh(name, &head);
	if(copy)
	{
		// always have an empty node allocated at the back.
		head->parent = copy;
		head->obj = copy->obj;
		head->vertex_data = copy->vertex_data;
		head->index_data = copy->index_data;
		head->vertex_count = copy->vertex_count;
		head->index_count = copy->index_count;
		zone::Q_memcpy(&head->buffer[0], &copy->buffer[0], sizeof(head->buffer));
		zone::Q_memcpy(&head->dset[0], &copy->dset[0], sizeof(head->dset));
		zone::Q_memcpy(&head->buffer_offset[0], &copy->buffer_offset[0], sizeof(head->buffer_offset));
		head->mat = (UniformMatrix*) control::UniformBufferDigress(sizeof(UniformMatrix), &head->buffer[3], &head->uniform_offset[0], &head->dset[0], 0);
		IdentityMatrix(head->mat->proj);
		IdentityMatrix(head->mat->model);
		IdentityMatrix(head->mat->view);
		head->collisionMesh = copy->collisionMesh;
		head->collisionShape = copy->collisionShape;
		head->colShape = copy->colShape;
		btTransform transform;
		transform.setIdentity();
		btVector3 inertia = btVector3(0.0f, 0.0f, 0.0f);
		head->colShape->calculateLocalInertia(100.f, inertia);
		btDefaultMotionState* motionState = new btDefaultMotionState(transform);
		btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(100.f, motionState, head->colShape, inertia);
		head->rigidBody = new btRigidBody(rigidBodyCI);
		dynamicsWorld->addRigidBody(head->rigidBody);
		head->rigidBody->setFriction(1.0f);

		head->next = (mesh_ent_t*) zone::Z_Malloc(sizeof(mesh_ent_t));
		head->next->prev = head;
	}
	else
	{
		error("mesh ent %s not found!", name);
	}
	return head;
}

void SetPosition(mesh_ent_t* copy, vec3_t pos)
{
	btTransform transform;
	copy->rigidBody->getMotionState()->getWorldTransform(transform);
	transform.setOrigin(btVector3(pos[0], pos[1], pos[2]));
	copy->rigidBody->getMotionState()->setWorldTransform(transform);
	copy->rigidBody->setCenterOfMassTransform(transform);
}

void MoveTo(char* name, vec3_t pos)
{
	mesh_ent_t* copy = GetMesh(name, nullptr);
	if(copy)
	{
		SetPosition(copy, pos);
	}
	else
	{
		error("mesh ent %s not found!", name);
	}
}


	btVector3 GetRayTo(int x, int y)
	{

		float top = 1.f;
		float bottom = -1.f;
		float nearPlane = 1.f;
		float tanFov = (top - bottom) * 0.5f / nearPlane;
		float fov = btScalar(2.0) * btAtan(tanFov);

		btVector3 camPos(cam.pos[0], cam.pos[1], cam.pos[2]);
		btVector3 camTarget(0,0,0);


		btVector3 rayFrom = camPos;
		btVector3 rayForward = (camTarget - camPos);
		float farPlane = 10000.f;
		rayForward *= farPlane;

		btVector3 rightOffset;
		btVector3 cameraUp = btVector3(0, 0, 0);
		cameraUp[1] = -1;

		btVector3 vertical = cameraUp;

		btVector3 hor;
		hor = rayForward.cross(vertical);
		hor.safeNormalize();
		vertical = hor.cross(rayForward);
		vertical.safeNormalize();

		float tanfov = tanf(0.5f * fov);

		hor *= 2.f * farPlane * tanfov;
		vertical *= 2.f * farPlane * tanfov;

		btScalar aspect;
		float width = (float)window_width;
		float height = (float)window_height;

		aspect = width / height;

		hor *= aspect;

		btVector3 rayToCenter = rayFrom + rayForward;
		btVector3 dHor = hor * 1.f / width;
		btVector3 dVert = vertical * 1.f / height;

		btVector3 rayTo = rayToCenter - 0.5f * hor + 0.5f * vertical;
		rayTo += btScalar(x) * dHor;
		rayTo -= btScalar(y) * dVert;
		return rayTo;
	}

	bool PickBody(const btVector3& rayFromWorld, const btVector3& rayToWorld)
	{
		p("From %f %f %f", rayFromWorld.getX(), rayFromWorld.getY(), rayFromWorld.getZ());
		p("To %f %f %f", rayToWorld.getX(), rayToWorld.getY(), rayToWorld.getZ());
		btCollisionWorld::ClosestRayResultCallback rayCallback(rayFromWorld, rayToWorld);

		rayCallback.m_flags |= btTriangleRaycastCallback::kF_UseGjkConvexCastRaytest;
		dynamicsWorld->rayTest(rayFromWorld, rayToWorld, rayCallback);
		if (rayCallback.hasHit())
		{
			btVector3 pickPos = rayCallback.m_hitPointWorld;
			btRigidBody* body = (btRigidBody*)btRigidBody::upcast(rayCallback.m_collisionObject);
			if (body)
			{
				//other exclusions?
				if (!(body->isStaticObject() || body->isKinematicObject()))
				{
					//btVector3 p = rayFromWorld.lerp(rayToWorld, rayCallback.m_closestHitFraction);
					//p("%f %f", p, p+rayCallback.m_hitNormalWorld);
					m_pickedBody = body;
					m_savedState = m_pickedBody->getActivationState();
					m_pickedBody->setActivationState(DISABLE_DEACTIVATION);
					printf("pickPos=%f,%f,%f\n",pickPos.getX(),pickPos.getY(),pickPos.getZ());
					btVector3 localPivot = body->getCenterOfMassTransform().inverse() * pickPos;
					btPoint2PointConstraint* p2p = new btPoint2PointConstraint(*body, localPivot);
					dynamicsWorld->addConstraint(p2p, true);
					m_pickedConstraint = p2p;
					btScalar mousePickClamping = 30.f;
					p2p->m_setting.m_impulseClamp = mousePickClamping;
					//very weak constraint for picking
					p2p->m_setting.m_tau = 0.001f;
				}
				printf("hit !\n");
			}

			//					pickObject(pickPos, rayCallback.m_collisionObject);
			m_oldPickingPos = rayToWorld;
			m_hitPos = pickPos;
			m_oldPickingDist = (pickPos - rayFromWorld).length();
			//p("%d", m_oldPickingDist);
			//add p2p
		}
		return false;
	}

	bool MovePickedBody(const btVector3& rayFromWorld, const btVector3& rayToWorld)
	{
		if (m_pickedBody && m_pickedConstraint)
		{
			btPoint2PointConstraint* pickCon = static_cast<btPoint2PointConstraint*>(m_pickedConstraint);
			if (pickCon)
			{
				//keep it at the same picking distance

				btVector3 newPivotB;

				btVector3 dir = rayToWorld - rayFromWorld;
				dir.normalize();
				dir *= m_oldPickingDist;

				newPivotB = rayFromWorld + dir;
				pickCon->setPivotB(newPivotB);
				return true;
			}
		}
		return false;
	}

void RemovePickingConstraint()
{
	if (m_pickedConstraint)
		{
			m_pickedBody->forceActivationState(m_savedState);
			m_pickedBody->activate();
			dynamicsWorld->removeConstraint(m_pickedConstraint);
			delete m_pickedConstraint;
			m_pickedConstraint = 0;
			m_pickedBody = 0;
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

	dynamicsWorld->setGravity(btVector3(0, 0, 0));
}

void StepPhysics()
{
	dynamicsWorld->stepSimulation(frametime, 0);
}

} //namespace entity

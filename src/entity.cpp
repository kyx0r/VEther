#include "entity.h"
#include "flog.h"
#include "window.h"

cam_ent_t cam;

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

} //namespace entity

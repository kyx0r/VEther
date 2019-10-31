#include "startup.h"
#include "mathlib.h"

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

namespace entity
{
void UpdateCamera();
void InitCamera();
void ViewMatrix(float matrix[16]);

} //namespace entity

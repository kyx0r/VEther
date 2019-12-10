#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(push_constant) uniform PushConsts {
	mat4 mvp;
	uint window_width;
	uint window_height;
} push_constants;

layout (location = 0) in vec3 pos;

layout (location = 0) out float height;

void main()
{
	height = pos.y;
	gl_Position = push_constants.mvp * vec4(pos, 1.0f);
}

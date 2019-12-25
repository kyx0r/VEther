#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(push_constant) uniform PushConsts {
	mat4 mvp;
	uint window_width;
	uint window_height;
} push_constants;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 translation_matrix;
} ubo;

layout (location = 0) in vec3 pos;

layout (location = 2) out float height;

void main()
{
        height = pos.y;
	gl_Position = push_constants.mvp * ubo.translation_matrix * vec4(-pos, 1.0f);
}

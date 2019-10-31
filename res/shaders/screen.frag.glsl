#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outputColor;

void main()
{
	outputColor = vec4(1.0, 0.0, 1.0, 1.0);
}
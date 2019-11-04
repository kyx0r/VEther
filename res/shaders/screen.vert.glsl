#version 450
#extension GL_ARB_separate_shader_objects : enable

const vec3 vertices[] =
{
	vec3(-1.0, -1.0, 0),
	vec3(-1.0, 1.0, 0),
	vec3(1.0, 1.0, 0),

	vec3(1.0, -1.0, 0),    
	vec3(-1.0, -1.0, 0),
	vec3(1.0, 1.0, 0),	
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inColor;

layout(location = 1) out vec2 fragTexCoord;
layout(location = 0) out vec3 fragColor;

void main()
{
//vertices[gl_VertexIndex]
   gl_Position = vec4(inPosition, 1.0);
   fragColor = inColor;
   fragTexCoord = inTexCoord;   
}
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

layout(push_constant) uniform PushConsts {
	mat4 mvp;
	uint window_width;
	uint window_height;
} push_constants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in uint inColor;

layout(location = 1) out vec2 fragTexCoord;
layout(location = 0) out vec4 fragColor;

void main()
{
   //vertices[gl_VertexIndex]

   float x = (inPosition.x - ((push_constants.window_width - 0.001) / 2)) / ((push_constants.window_width - 0.001) / 2);
   float y = (inPosition.y - ((push_constants.window_height - 0.001) / 2)) / ((push_constants.window_height - 0.001) / 2);

   vec4 conv = vec4((inColor & 0xFF000000) >> 24, (inColor & 0x0000FF00) >> 8, (inColor & 0x00FF0000) >> 16, (inColor & 0x000000FF) >> 0);
   conv.r = conv.r / 255.0;
   conv.g = conv.g / 255.0;
   conv.b = conv.b / 255.0;
   conv.a = conv.a / 255.0;

   gl_Position = vec4(x, y, inPosition.z, 1.0);
   
   fragColor = conv;
   fragTexCoord = inTexCoord;   
}
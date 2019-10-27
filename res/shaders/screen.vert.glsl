#version 450
#extension GL_ARB_separate_shader_objects : enable

const vec2 madd=vec2(0.5,0.5);
layout(location = 1) in vec2 vertexIn;

layout(location = 1) out vec2 textureCoord;

void main()
{
   textureCoord = vertexIn.xy*madd+madd; // scale vertex attribute to [0-1] range
   gl_Position = vec4(vertexIn.xy,0.0,1.0);
}
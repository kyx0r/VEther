#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 1) in vec2 textureCoord;

layout(location = 0) out vec4 outColor;

void main()
{
   vec4 color1 = texture(texSampler, textureCoord);
   outColor = color1;
}
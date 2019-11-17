#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;


layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(set = 1, binding = 0) uniform sampler2D texSampler;


void main()
{
	
//	outColor = vec4(0.1960, 0.1960, 0.1960, 1.0);
	
	//outColor = vec4(fragColor.r, fragColor.g, fragColor.b, fragColor.a);	
	outColor = fragColor;
	//outColor = texture(texSampler, fragTexCoord); //* vec4(fragColor, 1.0);
}
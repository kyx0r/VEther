#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;


layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(set = 1, binding = 0) uniform sampler2D texSampler;


void main()
{
	//when we execute the texture, fc.r should be used, when just normal rects fragColor.a should be used.
	vec4 afc;
	if(fragTexCoord.x > 1.0)
	{
		afc = vec4(fragColor.r, fragColor.g, fragColor.b, fragColor.a);
	}
	else
	{
		vec4 fc = texture(texSampler, fragTexCoord);
		afc = vec4(fragColor.r, fragColor.g, fragColor.b, fc.r) * fragColor.a;	 
	}
	
	outColor = afc;	     	 
}

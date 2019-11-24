#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;


layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(set = 1, binding = 0) uniform sampler2D texSampler;


void main()
{
	
	vec4 fc = texture(texSampler, fragTexCoord);
	
	vec4 afc = vec4(fragColor.r, fragColor.g, fragColor.b, fc.r+fragColor.a);
	 if(fc.rgb == vec3(0.0,0.0,0.0) && fragColor.a < 1)
	 {
	 	discard;
		//outColor = vec4(1.0,0.0,0.0,1.0);
	 }
	 else
	 {
		outColor = afc;
	 }
	//when we execute the texture, fc.r should be used, when just normal rects fragColor.a should be used.


	//outColor = mix(vec4(1.0, 1.0, 1.0, fc.r), fragColor, fragColor.a);
		
	//outColor = fragColor;


	//outColor = texture(texSampler, fragTexCoord); //* vec4(fragColor, 1.0);
}
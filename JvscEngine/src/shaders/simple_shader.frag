#version 460

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outColor;

layout (push_constant) uniform SimplePushConstants
{
	mat4 transform;
	vec3 color;
} push;

void main()
{
	outColor = vec4(inColor, 1.0);	
}
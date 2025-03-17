#version 460

layout (location = 0) out vec4 outColor;

layout (push_constant) uniform SimplePushConstants
{
	mat2 transform;
	vec2 offset;
	vec3 color;
} push;

void main()
{
	outColor = vec4(push.color, 1.0);	
}
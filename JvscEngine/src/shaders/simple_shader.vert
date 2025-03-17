#version 460

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec3 inColor;


layout (push_constant) uniform SimplePushConstants
{
	mat2 transform;
	vec2 offset;
	vec3 color;
} push;

void main()
{
	gl_Position = vec4(push.transform * inPosition + push.offset, 0.0, 1.0);
}
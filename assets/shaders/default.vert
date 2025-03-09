#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 0) out vec3 outColor;

layout (binding = 0) uniform CameraData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
} camera;

void main()
{
    gl_Position = camera.viewproj * vec4(vPosition.x, vPosition.y, vPosition.z, 1.0f);
    outColor = vColor;
}

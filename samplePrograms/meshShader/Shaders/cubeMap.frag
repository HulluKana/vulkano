#version 460

#extension GL_GOOGLE_include_directive : enable

#include"../include/host_device.hpp"

layout (location = 0) in vec3 fragDir;
layout (location = 0) out vec4 FragColor;

layout(set = 0, binding = 0) uniform samplerCubeArray cubeMap;
layout (early_fragment_tests) in;

void main()
{
    FragColor = texture(cubeMap, vec4(fragDir, 0.0));
}

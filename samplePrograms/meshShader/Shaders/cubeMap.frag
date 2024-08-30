#version 460

#extension GL_GOOGLE_include_directive : enable

#include"../include/host_device.hpp"

layout (location = 0) in vec3 fragDir;
layout (location = 0) out vec4 FragColor;

layout(set = 0, binding = 0) uniform samplerCube cubeMap;
layout (early_fragment_tests) in;

void main()
{
    FragColor = texture(cubeMap, fragDir);
    FragColor = vec4(pow(FragColor.x, 150.0) / 2.0, FragColor.yzw);
}

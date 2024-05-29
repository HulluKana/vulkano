#version 460

#extension GL_GOOGLE_include_directive : enable

#include"../include/host_device.hpp"
#include"../../../essentials/include/vul_scene.hpp"

layout (location = 0) in vec3 fragPosWorld;

layout (location = 0) out vec4 FragColor;

layout (early_fragment_tests) in;

void main()
{
    FragColor = vec4(0.8, 0.8, 0.8, 1.0);
}

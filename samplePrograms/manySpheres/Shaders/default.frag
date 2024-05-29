#version 460

#extension GL_GOOGLE_include_directive : enable

#include"../include/host_device.hpp"

layout (location = 0) in flat int idx;

layout (location = 0) out vec4 FragColor;

layout (set = 0, binding = 1) readonly buffer ObjDatas {ObjData objDatas[];};

layout (early_fragment_tests) in;

void main()
{
    FragColor = objDatas[idx].color;
}

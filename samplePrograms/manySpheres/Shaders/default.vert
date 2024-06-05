#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include"../include/host_device.hpp"

layout (location = 0) in vec3 position;

layout (location = 0) out flat int idx;

layout (set = 0, binding = 0) uniform Ub {RasUbo ubo;};
layout (set = 0, binding = 1) readonly buffer ObjDatas {ObjData objDatas[];};

void main()
{
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * vec4(position + objDatas[gl_InstanceIndex].pos.xyz, 1.0f);
    idx = gl_InstanceIndex;
}

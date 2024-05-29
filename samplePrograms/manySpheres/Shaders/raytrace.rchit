#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "../include/host_device.hpp"

layout(binding = 1, set = 0) uniform accelerationStructureEXT tlas;
layout(binding = 2, set = 0) readonly buffer ColorBuf {vec4 colors[];};

layout(location = 0) rayPayloadInEXT vec3 prd;

void main()
{
    prd = colors[gl_InstanceID * 5000000 + gl_PrimitiveID].xyz;
}

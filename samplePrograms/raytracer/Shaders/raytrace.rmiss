#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "../include/host_device.hpp"
#include "common.glsl"

layout(binding = 1, set = 0) uniform Ubo {GlobalUbo ubo;};
layout(binding = 12, set = 0) uniform samplerCube enviromentMap;

layout(location = 0) rayPayloadInEXT payload prd;

void main()
{
    const vec3 dir = vec3(-gl_WorldRayDirectionEXT.x, gl_WorldRayDirectionEXT.y, gl_WorldRayDirectionEXT.z);
    prd.hitValue = texture(enviromentMap, dir);
    prd.emission = vec3(0.0);
}

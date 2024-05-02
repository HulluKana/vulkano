#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "../include/host_device.hpp"
#include "common.glsl"

layout(binding = 1, set = 0) uniform Ubo {GlobalUbo ubo;};

layout(location = 0) rayPayloadInEXT payload prd;

void main()
{
    prd.hitValue = vec4(ubo.ambientLightColor.xyz * ubo.ambientLightColor.w, 1.0);
}

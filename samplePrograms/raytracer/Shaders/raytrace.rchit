#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

layout(location = 0) rayPayloadInEXT payload prd;
hitAttributeEXT vec3 attribs;

void main()
{
    prd.hitValue = vec3(0.8, 0.6, 0.4);
}

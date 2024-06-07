#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "../include/host_device.hpp"

layout(binding = 1, set = 0) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadInEXT vec3 prd;

void main()
{
    const vec3 pos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    const vec3 color = (pos + float(VOLUME_LEN / 2)) / float(VOLUME_LEN);
    prd = color;
}

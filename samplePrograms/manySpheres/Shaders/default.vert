#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include"../include/host_device.hpp"

layout (location = 0) in vec3 position;

layout (location = 0) out vec3 fragPosWorld;

layout (set = 0, binding = 0) uniform Ub {Ubo ubo;};
layout (set = 0, binding = 1) readonly buffer Trans {mat4 transforms[];};

void main()
{
    const mat4 tranform = transforms[gl_InstanceIndex];
    const vec4 worldPosition = tranform * vec4(position, 1.0);
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * tranform * vec4(position, 1.0f);
    fragPosWorld = worldPosition.xyz;
}

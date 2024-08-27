#version 460

#extension GL_GOOGLE_include_directive : enable

#include"../include/host_device.hpp"

layout (location = 0) in vec3 position;
layout (location = 0) out vec3 fragDir;

layout(push_constant) uniform Push{CubeMapPushConstant push;};

void main()
{
    const vec4 pos = push.projectionMatrix * push.originViewMatrix * vec4(position, 1.0);
    gl_Position = vec4(pos.xy, 0.99999 * pos.w, pos.w);
    fragDir = position;
}

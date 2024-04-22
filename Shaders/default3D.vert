#version 460

#extension GL_GOOGLE_include_directive : enable

#include"../vulkano/essentials/include/vul_host_device.hpp"

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 tangent;
layout (location = 3) in vec2 uv;

layout (location = 0) out vec3 fragPosWorld;
layout (location = 1) out vec3 fragNormalWorld;
layout (location = 2) out vec4 fragTangentWorld;
layout (location = 3) out vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform Ubo {GlobalUbo ubo;};

layout (push_constant) uniform Push{DefaultPushConstant push;};

void main()
{
    vec4 worldPosition = push.modelMatrix * vec4(position, 1.0);
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * push.modelMatrix * vec4(position, 1.0f);
    fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
    fragTangentWorld = vec4(normalize(mat3(push.normalMatrix) * tangent.xyz), tangent.w);
    fragPosWorld = worldPosition.xyz;
    fragTexCoord = uv;
}

#version 460

#extension GL_GOOGLE_include_directive : enable

#include"../vulkano/essentials/include/vul_host_device.hpp"

layout (location = 0) in vec2 fragPos;
layout (location = 1) in vec2 fragTexCoord;

layout (location = 0) out vec4 FragColor;

layout (set = 0, binding = 0) readonly buffer AlphaBuffer{ABuffer aBuffer[];};
layout (set = 0, binding = 1, r32ui) uniform uimage2D aBufferHeads;

struct UnpackedABuffer {
    vec3 color;
    float alpha;
    float depth;
};

void main()
{
    uint offset = imageLoad(aBufferHeads, ivec2(gl_FragCoord.xy)).r;
    if (offset == 0) discard;

    UnpackedABuffer frags[OIT_LAYERS];
    uint fragCount = 0;
    while (offset != uint(0) && fragCount < OIT_LAYERS) {
        ABuffer stored = aBuffer[offset];
        vec4 storedColorFactor = unpackUnorm4x8(stored.color);
        vec4 storedReflection = unpackUnorm4x8(stored.reflectionColor);
        UnpackedABuffer unpacked;
        unpacked.color = storedColorFactor.xyz;
        unpacked.alpha = storedColorFactor.w;
        unpacked.depth = stored.depth;
        frags[fragCount] = unpacked;
        offset = stored.next;
        fragCount++;
    }

    bool keepSorting = true;
    while (keepSorting) {
        keepSorting = false;
        for (uint i = 0; i < fragCount - 1; i++) {
            if (frags[i].depth < frags[i + 1].depth) {
                UnpackedABuffer temp = frags[i + 1];
                frags[i + 1] = frags[i];
                frags[i] = temp;
                keepSorting = true;
            }
        }
    }

    vec3 color = frags[0].color;
    float alphaInverted = 1.0 - frags[0].alpha;
    for (uint i = 1; i < fragCount; i++) {
        // color *= frags[i].color;
        color = color * (1.0 - frags[i].alpha) + frags[i].color * frags[i].alpha;
        alphaInverted *= 1.0 - frags[i].alpha;
    }
    FragColor = vec4(color, 1.0 - alphaInverted);

    imageStore(aBufferHeads, ivec2(gl_FragCoord.xy), uvec4(0));
}

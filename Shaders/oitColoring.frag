#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include"../vulkano/essentials/include/vul_host_device.hpp"
#include"common.glsl"

layout (location = 0) in vec3 fragPosWorld;
layout (location = 1) in vec3 fragNormalWorld;
layout (location = 2) in vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform Ubo {GlobalUbo ubo;};

layout (set = 0, binding = 1) uniform sampler2D texSampler[];
layout(set = 0, binding = 2) readonly buffer MaterialBuffer{PackedMaterial m[];} matBuf;
layout(set = 0, binding = 3) uniform sampler2D multipleBounce2dImg;
layout(set = 0, binding = 4) uniform sampler1D multipleBounce1dImg;

layout (set = 1, binding = 0) buffer AlphaBuffer{ABuffer aBuffer[];};
layout (set = 1, binding = 1, r32ui) uniform uimage2D aBufferHeads;
layout (set = 1, binding = 2) buffer AlphaBufferCounter{uint aBufferCounter;};
layout (set = 1, binding = 3) uniform sampler2D depthImages[];

layout (push_constant) uniform Push{OitPushConstant push;};

void main()
{
    if (texture(depthImages[push.depthImageIdx], vec2(gl_FragCoord.x / push.width, gl_FragCoord.y / push.height)).x < gl_FragCoord.z) discard;

    struct Material{
        vec3 color;
        float alpha;
        float roughness;
        int colorTextureIndex; 
    } mat;
    if (push.matIdx > -1){
        PackedMaterial packedMat = matBuf.m[push.matIdx];
        mat.color = packedMat.colorFactor.xyz;
        mat.alpha = packedMat.colorFactor.w;
        mat.roughness = packedMat.roughness;
        mat.colorTextureIndex = packedMat.colorTextureIndex;
    } else{
        mat.color = vec3(0.8);
        mat.alpha = 1.0;
        mat.roughness = 0.5;
        mat.colorTextureIndex = -1;
    }

    float epsilon = 0.0001;
    vec3 rawColor;
    if (mat.colorTextureIndex >= 0) rawColor = texture(texSampler[mat.colorTextureIndex], fragTexCoord).xyz;
    else rawColor = sRGBToAlbedo(mat.color);

    vec3 surfaceNormal = normalize(fragNormalWorld);
    vec3 viewDirection = normalize(ubo.cameraPosition.xyz - fragPosWorld);
    if (dot(surfaceNormal, viewDirection) < 0.0) surfaceNormal = -surfaceNormal;
    vec3 color = vec3(0.0);
    const vec3 specularColor = vec3(0.03);
    for (int i = 0; i < ubo.numLights; i++){
        vec3 lightPos = ubo.lightPositions[i].xyz;
        vec4 lightColor = ubo.lightColors[i];

        vec3 directionToLight = lightPos - fragPosWorld;
        float attenuation = 1.0 / dot(directionToLight, directionToLight);
        directionToLight = normalize(directionToLight);

        vec3 colorFromThisLight = BRDF(surfaceNormal, viewDirection, directionToLight, specularColor, mat.roughness);
        colorFromThisLight *= sRGBToAlbedo(lightColor.xyz * lightColor.w) * attenuation;
        color += colorFromThisLight;
    }

    color += sRGBToAlbedo(ubo.ambientLightColor.xyz * ubo.ambientLightColor.w) * specularColor;

    const uint newOffset = atomicAdd(aBufferCounter, 1) + 1;
    const uint oldOffset = imageAtomicExchange(aBufferHeads, ivec2(gl_FragCoord.xy), newOffset);
    ABuffer storeValue;
    storeValue.color = packUnorm4x8(vec4(albedoToSRGB(rawColor), mat.alpha));
    storeValue.reflectionColor = packUnorm4x8(vec4(albedoToSRGB(color), mat.alpha));
    storeValue.depth = gl_FragCoord.z;
    storeValue.next = oldOffset;
    aBuffer[newOffset] = storeValue;
}

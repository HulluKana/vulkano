#version 460

#extension GL_GOOGLE_include_directive : enable

#include"../vulkano/Backend/Headers/vul_host_device.hpp"
#include"allCommon.glsl"

layout (location = 0) in vec3 fragPosWorld;
layout (location = 1) in vec3 fragNormalWorld;
layout (location = 2) in vec2 fragTexCoord;

layout (location = 0) out vec4 FragColor;

layout(set = 0, binding = 0) uniform Ubo {GlobalUbo ubo;};

layout (set = 0, binding = 1) uniform sampler2D texSampler[MAX_TEXTURES];
layout(set = 0, binding = 2) readonly buffer MaterialBuffer{PackedMaterial m[];} matBuf;

layout (push_constant) uniform Push{PushConstant push;};

float lambda(vec3 someVector, vec3 surfaceNormal, float roughness)
{
    const float dotP = dot(surfaceNormal, someVector);
    const float aPow2 = (dotP * dotP) / (roughness * roughness * (1.0 - dotP * dotP));
    return (sqrt(1.0 + 1.0 / aPow2) - 1.0) / 2.0;
}

vec3 BRDF(vec3 surfaceNormal, vec3 viewDirection, vec3 lightDirection, vec3 specularColor, float roughness)
{
    if (dot(surfaceNormal, viewDirection) <= 0.0 || dot(surfaceNormal, lightDirection) <= 0.0) return vec3(0.0);
    const float pi = 3.14159265359;
    const vec3 halfVector = normalize(lightDirection + viewDirection);
    const float dotHalfNorm = dot(halfVector, surfaceNormal);
    if (dot(halfVector, viewDirection) <= 0.0 || dot(halfVector, lightDirection) <= 0.0 || dotHalfNorm <= 0.0) return vec3(0.0);
    const vec3 freshnelColor = specularColor + (vec3(1.0) - specularColor) * pow((1.0 - max(0.0, dot(surfaceNormal, lightDirection))), 5.0);
    const float visibleFraction = 1.0 / (lambda(viewDirection, surfaceNormal, roughness) + lambda(lightDirection, surfaceNormal, roughness));
    const float roughnessPow2 = roughness * roughness;
    const float whatDoICallThis = (1.0 + dotHalfNorm * dotHalfNorm * (roughnessPow2 - 1.0) * (roughnessPow2 - 1.0));
    const float ggx = (1.0 * roughnessPow2) / (pi * whatDoICallThis);
    return (freshnelColor * visibleFraction * ggx) / (4.0 * abs(dot(surfaceNormal, lightDirection) * abs(dot(surfaceNormal, viewDirection))));
}

void main()
{
    Material mat;
    if (push.matIdx > -1){
        PackedMaterial packedMat = matBuf.m[push.matIdx];
        mat.color = packedMat.colorFactor.xyz;
        mat.alpha = packedMat.colorFactor.w;
        mat.emissiveColor = packedMat.emissiveFactor.xyz;
        mat.emissiveStrength = packedMat.emissiveFactor.w;
        mat.roughness = packedMat.roughness;
        mat.colorTextureIndex = packedMat.colorTextureIndex;
    } else{
        mat.color = vec3(0.8);
        mat.alpha = 1.0;
        mat.emissiveColor = vec3(0.0);
        mat.emissiveStrength = 0.0;
        mat.roughness = 0.5;
        mat.colorTextureIndex = -1;
    }

    float epsilon = 0.0001;
    vec3 rawColor;
    if (mat.colorTextureIndex >= 0 && mat.colorTextureIndex < MAX_TEXTURES) rawColor = texture(texSampler[mat.colorTextureIndex], fragTexCoord).xyz;
    else rawColor = mat.color;

    vec3 surfaceNormal = normalize(fragNormalWorld);
    vec3 viewDirection = normalize(ubo.cameraPosition.xyz - fragPosWorld);
    vec3 color = vec3(0.0);
    for (int i = 0; i < ubo.numLights; i++){
        vec3 lightPos = ubo.lightPositions[i].xyz;
        vec4 lightColor = ubo.lightColors[i];

        vec3 directionToLight = lightPos - fragPosWorld;
        float attenuation = 1.0 / dot(directionToLight, directionToLight);
        directionToLight = normalize(directionToLight);

        vec3 colorFromThisLight = BRDF(surfaceNormal, viewDirection, directionToLight, vec3(56.0 / 255.0), mat.roughness);
        colorFromThisLight *= lightColor.xyz * lightColor.w * attenuation;
        color += colorFromThisLight;
    }

    color += ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    if (mat.emissiveStrength > 0.01) color += mat.emissiveColor * mat.emissiveStrength;

    color.x = -1.0 / exp(color.x) + 1.0;
    color.y = -1.0 / exp(color.y) + 1.0;
    color.z = -1.0 / exp(color.z) + 1.0;
    FragColor = vec4(color, 1.0);
}

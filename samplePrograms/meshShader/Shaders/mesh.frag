#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable
#extension GL_EXT_nonuniform_qualifier : enable


#include"../include/host_device.hpp"
#include"../../../include/vul_scene.hpp"

layout (location = 0) in vec3 fragPosWorld;
layout (location = 1) in vec3 fragNormalWorld;
layout (location = 2) in vec4 fragTangentWorld;
layout (location = 3) in vec2 fragTexCoord;
layout (location = 4) in flat uint matIdx;

layout (location = 0) out vec4 FragColor;

layout(set = 0, binding = 0) uniform UniformBuffer {Ubo ubo;};
layout(set = 0, binding = 1) uniform sampler2D textures[];
layout(set = 0, binding = 2) readonly buffer MaterialBuffer{PackedMaterial materials[];};
layout(set = 0, binding = 12) uniform samplerCubeArray shadowMap;

layout (early_fragment_tests) in;

vec3 sRGBToAlbedo(vec3 sRGB)
{
    const vec3 prePow = (sRGB + vec3(0.055)) / 1.055;
    return vec3(pow(prePow.x, 2.4), pow(prePow.y, 2.4), pow(prePow.z, 2.4));
}

vec3 albedoToSRGB(vec3 albedo)
{
    const vec3 prePow = albedo * 1.055;
    return vec3(pow(prePow.x, 0.41667), pow(prePow.y, 0.41667), pow(prePow.z, 0.41167)) - vec3(0.055);
}

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
    const vec3 freshnelColor = specularColor + (vec3(1.0) - specularColor) * pow((1.0 - dot(halfVector, lightDirection)), 5.0);
    const float visibleFraction = 1.0 / (1.0 + lambda(viewDirection, surfaceNormal, roughness) + lambda(lightDirection, surfaceNormal, roughness));
    const float roughnessPow2 = roughness * roughness;
    const float whatDoICallThis = 1.0 + dotHalfNorm * dotHalfNorm * (roughnessPow2 - 1.0);
    const float ggx = roughnessPow2 / (pi * whatDoICallThis * whatDoICallThis);
    return (freshnelColor * visibleFraction * ggx) / (4.0 * dot(surfaceNormal, lightDirection * dot(surfaceNormal, viewDirection)));
}

vec3 diffBRDF(vec3 surfaceNormal, vec3 viewDirection, vec3 lightDirection, vec3 specularColor, vec3 diffuseColor)
{
    const float pi = 3.14159265359;
    const float nl = dot(surfaceNormal, lightDirection);
    const float nv = dot(surfaceNormal, viewDirection);
    if (nl <= 0.0 || nv <= 0.0) return vec3(0.0);
    return 21.0 / (20.0 * pi) * (vec3(1.0) - specularColor) * diffuseColor * (1.0 - pow(1.0 - nl, 5.0)) * (1.0 - pow(1.0 - nv, 5.0)); 
}

bool isInShadow(vec3 worldPos, vec3 lightPos, float lightRange, uint lightIdx)
{
    const vec3 dir = worldPos - lightPos;
    const vec3 absDir = abs(dir);
    const float localZComp = max(absDir.x, max(absDir.y, absDir.z));
    const float far = lightRange;
    const float near = 0.001;
    const float normZComp = -(far / (near - far) - (near * far) / (near - far) / localZComp);
    const float depth = texture(shadowMap, vec4(dir, lightIdx)).r;
    const float bias = 0.00001;
    return normZComp - bias <= depth ? false : true;
}

void main()
{
    const float epsilon = 0.0001;

    const PackedMaterial mat = materials[matIdx];
    vec3 rawColor;
    if (mat.colorTextureIndex >= 0) rawColor = texture(textures[mat.colorTextureIndex], fragTexCoord).xyz;
    else rawColor = sRGBToAlbedo(mat.colorFactor.xyz);

    vec3 surfaceNormal = normalize(fragNormalWorld);
    if (mat.normalTextureIndex >= 0) {
        const vec3 bitangent = normalize(cross(surfaceNormal, normalize(fragTangentWorld.xyz)) * fragTangentWorld.w);
        const mat3 TBN = mat3(normalize(fragTangentWorld.xyz), bitangent, surfaceNormal);
        surfaceNormal = normalize(TBN * normalize(texture(textures[mat.normalTextureIndex], fragTexCoord).xyz * 2.0 - vec3(1.0)));
    }

    float roughness = mat.roughness;
    float metalliness = mat.metalliness;
    if (mat.roughnessMetallicTextureIndex >= 0) {
        const vec2 roughnessMetallic = texture(textures[mat.roughnessMetallicTextureIndex], fragTexCoord).yz;
        roughness = roughnessMetallic.x;
        metalliness = roughnessMetallic.y;
    }

    vec3 viewDirection = normalize(ubo.cameraPosition.xyz - fragPosWorld);
    vec3 color = vec3(0.0);
    const vec3 specularColor = mix(vec3(0.03), rawColor, metalliness);
    const vec3 diffuseColor = mix(rawColor, vec3(0.0), metalliness);
    for (uint i = 0; i < ubo.lightCount; i++){
        const vec3 lightPos = ubo.lightPositions[i].xyz;
        const float lightrange = ubo.lightPositions[i].w;
        const vec4 lightColor = ubo.lightColors[i];

        vec3 lightDir = lightPos - fragPosWorld;
        const float lightDstSquared = dot(lightDir, lightDir);
        const float lightDst = sqrt(lightDstSquared);
        if (lightDst > lightrange) continue;

        lightDir = normalize(lightDir);
        if (dot(surfaceNormal, lightDir) <= 0.0) continue;

        if (isInShadow(fragPosWorld, lightPos, lightrange, i)) continue;

        vec3 colorFromThisLight = vec3(0.0);
        colorFromThisLight += BRDF(surfaceNormal, viewDirection, lightDir, specularColor, roughness);
        colorFromThisLight += diffBRDF(surfaceNormal, viewDirection, lightDir, specularColor, diffuseColor);
        colorFromThisLight *= sRGBToAlbedo(lightColor.xyz * lightColor.w) / lightDstSquared;
        color += colorFromThisLight;
    }

    color += sRGBToAlbedo(ubo.ambientLightColor.xyz * ubo.ambientLightColor.w) * rawColor;

    FragColor = vec4(albedoToSRGB(color), 1.0);
}

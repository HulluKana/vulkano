#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"
#include "../include/host_device.hpp"
#include"../../../essentials/include/vul_scene.hpp"

layout(binding = 1, set = 0) uniform Ubo {GlobalUbo ubo;};
layout(binding = 2, set = 0) readonly buffer Indices            {uint indices[];};
layout(binding = 3, set = 0, scalar) readonly buffer Vertices   {vec3 vertices[];};
layout(binding = 4, set = 0, scalar) readonly buffer Normals    {vec3 normals[];};
layout(binding = 5, set = 0) readonly buffer Tangents           {vec4 tangents[];};
layout(binding = 6, set = 0) readonly buffer Uvs                {vec2 uvs[];};
layout(binding = 7, set = 0) readonly buffer Materials          {PackedMaterial materials[];};
layout(binding = 8, set = 0) readonly buffer PrimInfos          {PrimInfo primInfos[];};
layout(binding = 9, set = 0) uniform sampler2D texSampler[];

layout(location = 0) rayPayloadInEXT payload prd;
hitAttributeEXT vec3 attribs;

struct Material{
    vec3 color;
    float alpha;
    vec3 emissiveColor;
    float emissiveStrength;
    float roughness;
    float metalliness;
    int colorTextureIndex; 
    int normalTextureIndex;
    int roughnessMetallicTextureIndex;
} mat;

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

float albedoToSRGB(float albedo)
{
    const float prePow = albedo * 1.055;
    return pow(prePow, 0.41667) - 0.055;
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

vec3 diffBRDF(vec3 normal, vec3 viewDirection, vec3 lightDirection, vec3 specularColor, vec3 diffuseColor)
{
    const float pi = 3.14159265359;
    const float nl = dot(normal, lightDirection);
    const float nv = dot(normal, viewDirection);
    if (nl <= 0.0 || nv <= 0.0) return vec3(0.0);
    return 21.0 / (20.0 * pi) * (vec3(1.0) - specularColor) * diffuseColor * (1.0 - pow(1.0 - nl, 5.0)) * (1.0 - pow(1.0 - nv, 5.0)); 
}

void getVertexInputs(out vec3 worldPos, out vec3 worldNormal, out vec4 worldTangent, out vec2 uv, out Material material, out float normalMapLod)
{
    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    PrimInfo primInfo = primInfos[gl_InstanceCustomIndexEXT];
    const uint indexOffset = primInfo.firstIndex + gl_PrimitiveID * 3;
    const uvec3 index = uvec3(indices[indexOffset], indices[indexOffset + 1], indices[indexOffset + 2]) + uvec3(primInfo.vertexOffset);

    const vec3 worldPos1 = vec3(gl_ObjectToWorldEXT * vec4(vertices[index.x], 1.0));
    const vec3 worldPos2 = vec3(gl_ObjectToWorldEXT * vec4(vertices[index.y], 1.0));
    const vec3 worldPos3 = vec3(gl_ObjectToWorldEXT * vec4(vertices[index.z], 1.0));
    worldPos = worldPos1 * barycentrics.x + worldPos2 * barycentrics.y + worldPos3 * barycentrics.z;

    const vec3 normal = normals[index.x] * barycentrics.x + normals[index.y] * barycentrics.y + normals[index.z] * barycentrics.z;
    worldNormal = normalize(vec3(normal * gl_WorldToObjectEXT));

    const vec4 tangent = tangents[index.x] * barycentrics.x + tangents[index.y] * barycentrics.y + tangents[index.z] * barycentrics.z;
    worldTangent = vec4(normalize(vec3(tangent.xyz * gl_WorldToObjectEXT)), tangent.w);

    const vec2 uv1 = uvs[index.x];
    const vec2 uv2 = uvs[index.y];
    const vec2 uv3 = uvs[index.z];
    uv = uv1 * barycentrics.x + uv2 * barycentrics.y + uv3 * barycentrics.z;

    if (primInfo.materialIndex >= 0) {
        PackedMaterial packedMat = materials[primInfo.materialIndex];
        material.color = packedMat.colorFactor.xyz;
        material.alpha = packedMat.colorFactor.w;
        material.emissiveColor = packedMat.emissiveFactor.xyz;
        material.emissiveStrength = packedMat.emissiveFactor.w;
        material.roughness = packedMat.roughness;
        material.metalliness = packedMat.metalliness;
        material.colorTextureIndex = packedMat.colorTextureIndex;
        material.normalTextureIndex = packedMat.normalTextureIndex;
        material.roughnessMetallicTextureIndex = packedMat.roughnessMetallicTextureIndex;
    } else{
        material.color = vec3(0.8);
        material.alpha = 1.0;
        material.emissiveColor = vec3(0.0);
        material.emissiveStrength = 0.0;
        material.roughness = 0.5;
        material.metalliness = 0.0;
        material.colorTextureIndex = -1;
        material.normalTextureIndex = -1;
        material.roughnessMetallicTextureIndex = -1;
    }

    // Screen space texture lod for selecting mip map calculated using equation 26 from
    // https://media.contentapi.ea.com/content/dam/ea/seed/presentations/2019-ray-tracing-gems-chapter-20-akenine-moller-et-al.pdf
    if (material.normalTextureIndex >= 0) {
        const vec2 textureDimensions = vec2(textureSize(texSampler[material.normalTextureIndex], 0));
        const float twiceTexelSpaceTriangleArea = textureDimensions.x * textureDimensions.y * abs((uv2.x - uv1.x) * (uv3.y - uv1.y) - (uv3.x - uv1.x) * (uv2.y - uv1.y));
        const float twiceWorldSpaceTriangleArea = length(cross(worldPos2 - worldPos1, worldPos3 - worldPos1));
        const float baseNormalMapLod = 0.5 * log2(twiceTexelSpaceTriangleArea / twiceWorldSpaceTriangleArea);
        normalMapLod = baseNormalMapLod + log2(ubo.pixelSpreadAngle * gl_HitTEXT * (1.0 / abs(dot(worldNormal, gl_WorldRayDirectionEXT))));
    }
}

void main()
{
    vec3 pos; 
    vec3 normal;
    vec4 tangent;
    vec2 uv;
    Material mat;
    float normalMapLod;
    getVertexInputs(pos, normal, tangent, uv, mat, normalMapLod);

    float epsilon = 0.0001;
    vec3 rawColor;
    if (mat.colorTextureIndex >= 0) rawColor = texture(texSampler[mat.colorTextureIndex], uv).xyz;
    else rawColor = sRGBToAlbedo(mat.color);

    if (mat.normalTextureIndex >= 0) {
        const vec3 bitangent = normalize(cross(normal, normalize(tangent.xyz)) * tangent.w);
        const mat3 TBN = mat3(normalize(tangent.xyz), bitangent, normal);
        normal = normalize(TBN * normalize(textureLod(texSampler[mat.normalTextureIndex], uv, normalMapLod).xyz * 2.0 - vec3(1.0)));
    }

    float roughness = mat.roughness;
    float metalliness = mat.metalliness;
    if (mat.roughnessMetallicTextureIndex >= 0) {
        const vec2 roughnessMetallic = texture(texSampler[mat.roughnessMetallicTextureIndex], uv).yz;
        roughness = roughnessMetallic.x;
        metalliness = roughnessMetallic.y;
    }

    vec3 viewDirection = normalize(ubo.cameraPosition.xyz - pos);
    vec3 color = vec3(0.0);
    const vec3 specularColor = mix(vec3(0.03), rawColor, metalliness);
    const vec3 diffuseColor = mix(rawColor, vec3(0.0), metalliness);
    for (int i = 0; i < ubo.numLights; i++){
        vec3 lightPos = ubo.lightPositions[i].xyz;
        vec4 lightColor = ubo.lightColors[i];

        vec3 directionToLight = lightPos - pos;
        float attenuation = 1.0 / dot(directionToLight, directionToLight);
        directionToLight = normalize(directionToLight);

        vec3 colorFromThisLight = vec3(0.0);
        colorFromThisLight += BRDF(normal, viewDirection, directionToLight, specularColor, roughness);
        colorFromThisLight += diffBRDF(normal, viewDirection, directionToLight, specularColor, diffuseColor);
        colorFromThisLight *= sRGBToAlbedo(lightColor.xyz * lightColor.w) * attenuation;
        color += colorFromThisLight;
    }

    color += sRGBToAlbedo(ubo.ambientLightColor.xyz * ubo.ambientLightColor.w) * rawColor;
    if (mat.emissiveStrength > 0.01) color += sRGBToAlbedo(mat.emissiveColor * mat.emissiveStrength);

    prd.hitValue = vec3(color);
}

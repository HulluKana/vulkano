#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"
#include "../include/host_device.hpp"
#include"../../../essentials/include/vul_scene.hpp"

layout(binding = 1, set = 0) readonly uniform Ubo                           {GlobalUbo ubo;};
layout(binding = 2, set = 0) readonly buffer Indices                        {uint indices[];};
layout(binding = 3, set = 0, scalar) readonly buffer Vertices               {vec3 vertices[];};
layout(binding = 4, set = 0, scalar) readonly buffer Normals                {vec3 normals[];};
layout(binding = 5, set = 0) readonly buffer Tangents                       {vec4 tangents[];};
layout(binding = 6, set = 0) readonly buffer Uvs                            {vec2 uvs[];};
layout(binding = 7, set = 0) readonly buffer Materials                      {PackedMaterial materials[];};
layout(binding = 8, set = 0) readonly buffer PrimInfos                      {PrimInfo primInfos[];};
layout(binding = 9, set = 0) readonly buffer LightInfos                     {LightInfo lightInfos[];};
layout(binding = 10, set = 0, scalar) readonly buffer ReservoirsBuffers     {ivec4 minPos; uvec4 dims; Reservoir data[];} reservoirs[RESERVOIR_HISTORY_LEN];
layout(binding = 11, set = 0, r32ui) writeonly uniform uimage3D hitCache;
layout(binding = 12, set = 0) uniform sampler2D texSampler[];
layout(binding = 14, set = 0) uniform accelerationStructureEXT tlas;

layout(push_constant) uniform Push{uint frameNumber;};

layout(location = 0) rayPayloadInEXT payload prd;
layout(location = 1) rayPayloadEXT float visibility;

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

float lambda(vec3 someVector, vec3 surfaceNormal, float roughness)
{
    const float dotP = dot(surfaceNormal, someVector);
    const float aPow2 = (dotP * dotP) / (roughness * roughness * (1.0 - dotP * dotP));
    return (sqrt(1.0 + 1.0 / aPow2) - 1.0) / 2.0;
}

vec3 BRDF(vec3 surfaceNormal, vec3 viewDirection, vec3 lightDirection, vec3 specularColor, float roughness)
{
    // if (dot(surfaceNormal, viewDirection) <= 0.0 || dot(surfaceNormal, lightDirection) <= 0.0) return vec3(0.0);
    const float pi = 3.14159265359;
    const vec3 halfVector = normalize(lightDirection + viewDirection);
    const float dotHalfNorm = dot(halfVector, surfaceNormal);
    // if (dot(halfVector, viewDirection) <= 0.0 || dot(halfVector, lightDirection) <= 0.0 || dotHalfNorm <= 0.0) return vec3(0.0);
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
    // if (nl <= 0.0 || nv <= 0.0) return vec3(0.0);
    return 21.0 / (20.0 * pi) * (vec3(1.0) - specularColor) * diffuseColor * (1.0 - pow(1.0 - nl, 5.0)) * (1.0 - pow(1.0 - nv, 5.0)); 
}

float randomFloat(inout uint state) {
    state = state * 747796405 + 2891336453;
    uint result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;
    result = (result >> 22) ^ result;
    return result / 4294967295.0;
}

void getVertexInputs(out vec3 worldPos, out vec3 worldNormal, out vec4 worldTangent, out vec2 uv, out Material material, out vec4 uvGradients)
{
    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    PrimInfo primInfo = primInfos[gl_InstanceCustomIndexEXT + gl_GeometryIndexEXT];
    const uint indexOffset = primInfo.firstIndex + gl_PrimitiveID * 3;
    const uvec3 index = uvec3(indices[indexOffset], indices[indexOffset + 1], indices[indexOffset + 2]) + uvec3(primInfo.vertexOffset);

    const vec3 worldPos1 = vec3(primInfo.transformMatrix * vec4(vertices[index.x], 1.0));
    const vec3 worldPos2 = vec3(primInfo.transformMatrix * vec4(vertices[index.y], 1.0));
    const vec3 worldPos3 = vec3(primInfo.transformMatrix * vec4(vertices[index.z], 1.0));
    worldPos = worldPos1 * barycentrics.x + worldPos2 * barycentrics.y + worldPos3 * barycentrics.z;

    const vec3 normal = normals[index.x] * barycentrics.x + normals[index.y] * barycentrics.y + normals[index.z] * barycentrics.z;
    worldNormal = normalize(vec3(mat3(primInfo.normalMatrix) * normal));

    const vec4 tangent = tangents[index.x] * barycentrics.x + tangents[index.y] * barycentrics.y + tangents[index.z] * barycentrics.z;
    worldTangent = vec4(normalize(vec3(mat3(primInfo.normalMatrix) * tangent.xyz)), tangent.w);

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
    // I'm using a modified version of it that uses gradients instead of lod, and it is from raytracing gems 2 book.
    // Additionally I got some anisotropic filtering stuff from https://jcgt.org/published/0010/01/01/

    // Gradients with anisotropic filtering
    const float epsilon = 0.0001;
    const vec3 rayDir = gl_WorldRayDirectionEXT;
    const float projectedConeRadius = 0.5 * (ubo.pixelSpreadAngle * gl_HitTEXT) / (abs(dot(worldNormal, rayDir)));

    const vec3 h1 = rayDir - dot(worldNormal, rayDir) * worldNormal;
    const vec3 a1 = projectedConeRadius / max(length(h1 - dot(rayDir, h1) * rayDir), epsilon) * h1;

    const vec3 h2 = cross(worldNormal, a1);
    const vec3 a2 = projectedConeRadius / max(length(h2 - dot(rayDir, h2) * rayDir), epsilon) * h2;

    const vec3 e1 = worldPos2 - worldPos1;
    const vec3 e2 = worldPos3 - worldPos1;
    const float oneOverAreaTriangle = 1.0 / dot(worldNormal, cross(e1, e2));

    vec3 eP = worldPos + a1 - worldPos1;
    const float u1 = dot(worldNormal, cross(eP, e2)) * oneOverAreaTriangle;
    const float v1 = dot(worldNormal, cross(e1, eP)) * oneOverAreaTriangle;
    const vec2 grad1 = (1.0 - u1 - v1) * uv1 + u1 * uv2 + v1 * uv3 - uv;

    eP = worldPos + a2 - worldPos1;
    const float u2 = dot(worldNormal, cross(eP, e2)) * oneOverAreaTriangle;
    const float v2 = dot(worldNormal, cross(e1, eP)) * oneOverAreaTriangle;
    const vec2 grad2 = (1.0 - u2 - v2) * uv1 + u2 * uv2 + v2 * uv3 - uv;

    uvGradients = vec4(grad1, grad2);

    // Gradients without anisotropic filtering
    /*
    const vec3 rayDir = gl_WorldRayDirectionEXT;
    const float projectedConeWidth = (ubo.pixelSpreadAngle * gl_HitTEXT) / (abs(dot(worldNormal, rayDir)));
    const float uvArea = abs((uv2.x - uv1.x) * (uv3.y - uv1.y) - (uv3.x - uv1.x) * (uv2.y - uv1.y));
    const float worldSpaceArea = length(cross(worldPos2 - worldPos1, worldPos3 - worldPos1));
    const float visibleAreaRatio = (projectedConeWidth * projectedConeWidth) / worldSpaceArea;
    const float visibleUvSideLength = sqrt(uvArea * visibleAreaRatio);
    uvGradients = vec4(visibleUvSideLength, 0.0, 0.0, visibleUvSideLength);
    */
}

void main()
{
    vec3 pos; 
    vec3 normal;
    vec4 tangent;
    vec2 uv;
    Material mat;
    vec4 uvGrads;
    getVertexInputs(pos, normal, tangent, uv, mat, uvGrads);
    uint state = (gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x) * frameNumber;

    float epsilon = 0.0001;
    vec3 rawColor;
    float alpha = mat.alpha;
    if (mat.colorTextureIndex >= 0) {
        const vec4 rgba = textureGrad(texSampler[mat.colorTextureIndex], uv, uvGrads.xy, uvGrads.wz);
        rawColor = rgba.xyz;
        alpha = rgba.w;
    }
    else rawColor = sRGBToAlbedo(mat.color);

    if (mat.normalTextureIndex >= 0) {
        const vec3 bitangent = normalize(cross(normal, normalize(tangent.xyz)) * tangent.w);
        const mat3 TBN = mat3(normalize(tangent.xyz), bitangent, normal);
        normal = normalize(TBN * normalize(textureGrad(texSampler[mat.normalTextureIndex], uv, uvGrads.xy, uvGrads.zw).xyz * 2.0 - vec3(1.0)));
    }

    float roughness = mat.roughness;
    float metalliness = mat.metalliness;
    if (mat.roughnessMetallicTextureIndex >= 0) {
        const vec2 roughnessMetallic = texture(texSampler[mat.roughnessMetallicTextureIndex], uv).yz;
        roughness = roughnessMetallic.x;
        metalliness = roughnessMetallic.y;
    }

    vec3 viewDirection = normalize(ubo.cameraPosition.xyz - pos);
    if (dot(normal, viewDirection) <= 0.0) {
        vec3 color = sRGBToAlbedo(ubo.ambientLightColor.xyz * ubo.ambientLightColor.w) * rawColor;
        if (mat.emissiveStrength > 0.01) color += sRGBToAlbedo(mat.emissiveColor * mat.emissiveStrength);

        prd.hitValue = vec4(albedoToSRGB(color), alpha);
        prd.emission = vec3(0.0);
        prd.pos = pos;
        return;
    }

    const ivec3 minPos = reservoirs[0].minPos.xyz;
    const uvec3 dims = reservoirs[0].dims.xyz;
    const uint gridX = int(floor(pos.x)) - minPos.x;
    const uint gridY = int(floor(pos.y)) - minPos.y;
    const uint gridZ = int(floor(pos.z)) - minPos.z;
    imageStore(hitCache, ivec3(gridX, gridY, gridZ), uvec4(frameNumber));

    const uint idx = (gridZ * dims.y * dims.x + gridY * dims.x + gridX) * RESERVOIRS_PER_CELL;
    float avgOfAllReservoirs = 0.0;
    for (uint i = idx; i < idx + RESERVOIRS_PER_CELL; i++) {
        const Reservoir reservoir = reservoirs[0].data[i];
        avgOfAllReservoirs += reservoir.averageWeight;
    }
    avgOfAllReservoirs /= float(RESERVOIRS_PER_CELL);

    Reservoir chosenOne;
    const vec3 specularColor = mix(vec3(0.03), rawColor, metalliness);
    const vec3 diffuseColor = mix(rawColor, vec3(0.0), metalliness);
    for (uint i = idx; i < idx + RESERVOIRS_PER_CELL; i++) {
        const Reservoir reservoir = reservoirs[frameNumber % RESERVOIR_HISTORY_LEN].data[i];
        const vec4 lightPos = lightInfos[reservoir.lightIdx].position;
        const vec4 lightColor = lightInfos[reservoir.lightIdx].color;

        vec3 lightDir = lightPos.xyz - pos;
        const float lightDstSquared = dot(lightDir, lightDir);
        lightDir = normalize(lightDir);
        const float lightDst = sqrt(lightDstSquared);
        const float attenuation = 1.0 / lightDstSquared;
        const LightInfo lightInfo = lightInfos[reservoir.lightIdx];
        const vec3 color = (BRDF(normal, viewDirection, lightDir, specularColor, roughness) + diffBRDF(normal, viewDirection, lightDir, specularColor, diffuseColor)) * sRGBToAlbedo(lightColor.xyz * lightColor.w) * attenuation;
        const float targetPdf = (color.x + color.y + color.z) / lightInfos.length();

        const float sourcePdf = reservoir.targetPdf / avgOfAllReservoirs;
        const float risWeight = targetPdf / sourcePdf;
        chosenOne.averageWeight += risWeight;
        if (randomFloat(state) < risWeight / chosenOne.averageWeight) {
            chosenOne.lightIdx = reservoir.lightIdx;
            chosenOne.targetPdf = targetPdf;
        }
    }
    chosenOne.averageWeight /= float(RESERVOIRS_PER_CELL);

    vec3 color = vec3(0.0);
    for (int i = 0; i < 1; i++) {
        const vec4 lightPos = lightInfos[chosenOne.lightIdx].position;
        const vec4 lightColor = lightInfos[chosenOne.lightIdx].color;

        vec3 lightDir = lightPos.xyz - pos;
        const float lightDstSquared = dot(lightDir, lightDir);
        const float lightDst = sqrt(lightDstSquared);
        const float attenuation = 1.0 / lightDstSquared;

        lightDir = normalize(lightDir);
        if (dot(normal, lightDir) <= 0.0) continue;

        visibility = 1.0;
        const uint flags = gl_RayFlagsSkipClosestHitShaderEXT | gl_RayFlagsTerminateOnFirstHitEXT;
        traceRayEXT(tlas,               // acceleration structure
                flags,                  // rayFlags
                0xFF,                   // cullMask
                0,                      // sbtRecordOffset
                0,                      // sbtRecordStride
                1,                      // missIndex
                pos,                    // ray origin
                0.001,                  // ray min range
                lightDir,               // ray direction
                lightDst,               // ray max range
                1                       // payload (location = 1)
            );
        if (visibility < 10.01) continue;

        vec3 colorFromThisLight = vec3(0.0);
        colorFromThisLight += BRDF(normal, viewDirection, lightDir, specularColor, roughness);
        colorFromThisLight += diffBRDF(normal, viewDirection, lightDir, specularColor, diffuseColor);
        colorFromThisLight *= sRGBToAlbedo(lightColor.xyz * lightColor.w) * attenuation * (visibility - 10.0) * chosenOne.averageWeight / chosenOne.targetPdf;
        color += colorFromThisLight;
    }

    color += sRGBToAlbedo(ubo.ambientLightColor.xyz * ubo.ambientLightColor.w) * rawColor;

    prd.hitValue = vec4(albedoToSRGB(color), alpha);
    prd.emission = mat.emissiveColor * mat.emissiveStrength;
    prd.pos = pos;
}

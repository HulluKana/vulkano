#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include"../vulkano/essentials/include/vul_host_device.hpp"
#include"common.glsl"

layout (location = 0) in vec3 fragPosWorld;
layout (location = 1) in vec3 fragNormalWorld;
layout (location = 2) in vec4 fragTangentWorld;
layout (location = 3) in vec2 fragTexCoord;

layout (location = 0) out vec4 FragColor;

layout(set = 0, binding = 0) uniform Ubo {GlobalUbo ubo;};

layout (set = 0, binding = 1) uniform sampler2D texSampler[];
layout(set = 0, binding = 2) readonly buffer MaterialBuffer{PackedMaterial m[];} matBuf;
layout(set = 0, binding = 3) uniform sampler2D multipleBounce2dImg;
layout(set = 0, binding = 4) uniform sampler1D multipleBounce1dImg;

layout (push_constant) uniform Push{DefaultPushConstant push;};

layout (early_fragment_tests) in;

vec3 multipleBounceBRDF(vec3 surfaceNormal, vec3 viewDirection, vec3 lightDirection, vec3 specularColor, float roughness)
{
    if (dot(surfaceNormal, viewDirection) <= 0.0 || dot(surfaceNormal, lightDirection) <= 0.0) return vec3(0.0);
    const float pi = 3.14159265359;
    const vec3 cosAvgFresnel = (specularColor * 20.0) / 21.0 + 1.0 / 21.0; 
    const float dirAlbLight = texture(multipleBounce2dImg, vec2(acos(dot(surfaceNormal, lightDirection)) / (pi / 2.0), roughness)).x;
    const float dirAlbView = texture(multipleBounce2dImg, vec2(acos(dot(surfaceNormal, viewDirection)) / (pi / 2.0), roughness)).x;
    const float cosAvgDirAlb = texture(multipleBounce1dImg, roughness).x;
    return (cosAvgFresnel * cosAvgDirAlb) / (pi * (1.0 - cosAvgDirAlb) * (1.0 - cosAvgFresnel * (1.0 - cosAvgDirAlb))) * (1.0 - dirAlbLight) * (1.0 - dirAlbView);
}

vec3 diffBRDF(vec3 surfaceNormal, vec3 viewDirection, vec3 lightDirection, vec3 specularColor, vec3 diffuseColor)
{
    const float pi = 3.14159265359;
    const float nl = dot(surfaceNormal, lightDirection);
    const float nv = dot(surfaceNormal, viewDirection);
    if (nl <= 0.0 || nv <= 0.0) return vec3(0.0);
    return 21.0 / (20.0 * pi) * (vec3(1.0) - specularColor) * diffuseColor * (1.0 - pow(1.0 - nl, 5.0)) * (1.0 - pow(1.0 - nv, 5.0)); 
}

void main()
{
    struct Material{
        vec3 color;
        float alpha;
        vec3 emissiveColor;
        float emissiveStrength;
        float roughness;
        float metalliness;
        float ior;
        int colorTextureIndex; 
        int normalTextureIndex;
        int roughnessMetallicTextureIndex;
    } mat;
    if (push.matIdx > -1){
        PackedMaterial packedMat = matBuf.m[push.matIdx];
        mat.color = packedMat.colorFactor.xyz;
        mat.alpha = packedMat.colorFactor.w;
        mat.emissiveColor = packedMat.emissiveFactor.xyz;
        mat.emissiveStrength = packedMat.emissiveFactor.w;
        mat.roughness = packedMat.roughness;
        mat.metalliness = packedMat.metalliness;
        mat.colorTextureIndex = packedMat.colorTextureIndex;
        mat.normalTextureIndex = packedMat.normalTextureIndex;
        mat.roughnessMetallicTextureIndex = packedMat.roughnessMetallicTextureIndex;
    } else{
        mat.color = vec3(0.8);
        mat.alpha = 1.0;
        mat.emissiveColor = vec3(0.0);
        mat.emissiveStrength = 0.0;
        mat.roughness = 0.5;
        mat.metalliness = 0.0;
        mat.colorTextureIndex = -1;
        mat.normalTextureIndex = -1;
        mat.roughnessMetallicTextureIndex = -1;
    }

    float epsilon = 0.0001;
    vec3 rawColor;
    if (mat.colorTextureIndex >= 0) rawColor = texture(texSampler[mat.colorTextureIndex], fragTexCoord).xyz;
    else rawColor = sRGBToAlbedo(mat.color);

    vec3 surfaceNormal = fragNormalWorld;
    if (mat.normalTextureIndex >= 0) {
        const vec3 bitangent = normalize(cross(fragNormalWorld, fragTangentWorld.xyz) * fragTangentWorld.w);
        const mat3 TBN = mat3(fragTangentWorld, bitangent, fragNormalWorld);
        surfaceNormal = normalize(TBN * (texture(texSampler[mat.normalTextureIndex], fragTexCoord).xyz * 2.0 - vec3(1.0)));
        surfaceNormal = surfaceNormal.yzx; // Dont ask me why. It just has to be this way
    }

    float roughness = mat.roughness;
    float metalliness = mat.metalliness;
    if (mat.roughnessMetallicTextureIndex >= 0) {
        const vec2 roughnessMetallic = texture(texSampler[mat.roughnessMetallicTextureIndex], fragTexCoord).yz;
        roughness = roughnessMetallic.x;
        metalliness = roughnessMetallic.y;
    }

    vec3 viewDirection = normalize(ubo.cameraPosition.xyz - fragPosWorld);
    vec3 color = vec3(0.0);
    const vec3 specularColor = mix(vec3(0.03), rawColor, metalliness);
    const vec3 diffuseColor = mix(rawColor, vec3(0.0), metalliness);
    for (int i = 0; i < ubo.numLights; i++){
        vec3 lightPos = ubo.lightPositions[i].xyz;
        vec4 lightColor = ubo.lightColors[i];

        vec3 directionToLight = lightPos - fragPosWorld;
        float attenuation = 1.0 / dot(directionToLight, directionToLight);
        directionToLight = normalize(directionToLight);

        vec3 colorFromThisLight = vec3(0.0);
        colorFromThisLight += BRDF(surfaceNormal, viewDirection, directionToLight, specularColor, roughness);
        colorFromThisLight += multipleBounceBRDF(surfaceNormal, viewDirection, directionToLight, specularColor, roughness);
        colorFromThisLight += diffBRDF(surfaceNormal, viewDirection, directionToLight, specularColor, diffuseColor);
        colorFromThisLight *= sRGBToAlbedo(lightColor.xyz * lightColor.w) * attenuation;
        color += colorFromThisLight;
    }

    color += sRGBToAlbedo(ubo.ambientLightColor.xyz * ubo.ambientLightColor.w) * rawColor;
    if (mat.emissiveStrength > 0.01) color += sRGBToAlbedo(mat.emissiveColor * mat.emissiveStrength);

    FragColor = vec4(color, 1.0);
}

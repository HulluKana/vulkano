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

    vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 specularLight = vec3(0.0);
    vec3 surfaceNormal = normalize(fragNormalWorld);

    vec3 viewDirection = normalize(ubo.cameraPosition.xyz - fragPosWorld);

    for (int i = 0; i < ubo.numLights; i++){
        vec3 lightPos = ubo.lightPositions[i].xyz;
        vec4 lightColor = ubo.lightColors[i];

        vec3 directionToLight = lightPos - fragPosWorld;
        float attenuation = 1.0 / dot(directionToLight, directionToLight);
        directionToLight = normalize(directionToLight);

        float cosAngleIncidence = dot(surfaceNormal, directionToLight);
        if (cosAngleIncidence > epsilon){
            // Converts the roughness ranging from 0 to 1 into specular exponent ranging from 1 to roughly 5,9
            const float specular = pow(mat.roughness + 0.2, 7.0) + pow(mat.roughness + 0.1, 3.0) + 1.0;

            vec3 intensity = lightColor.xyz * lightColor.w * attenuation;
            diffuseLight += intensity * cosAngleIncidence;

            vec3 halfAngle = normalize(directionToLight + viewDirection);
            float blinnTerm = dot(surfaceNormal, halfAngle);
            blinnTerm = clamp(blinnTerm, 0, 1);
            blinnTerm = pow(blinnTerm, specular);
            specularLight += intensity * blinnTerm;
        }
    }

    vec3 color = diffuseLight * rawColor + specularLight * rawColor;
    if (mat.emissiveStrength > 0.01) color += mat.emissiveColor * mat.emissiveStrength;

    color.x = -1.0 / exp(color.x) + 1.0;
    color.y = -1.0 / exp(color.y) + 1.0;
    color.z = -1.0 / exp(color.z) + 1.0;
    FragColor = vec4(color, 1.0);
}

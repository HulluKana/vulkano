#version 460

layout (location = 0) in vec3 fragPosWorld;
layout (location = 1) in vec3 fragNormalWorld;
layout (location = 2) in vec2 fragTexCoord;

layout (location = 0) out vec4 FragColor;

const int maxLights = 10;
layout (set = 0, binding = 0) uniform GlobalUbo {
    mat4 projectionViewMatrix;
    vec4 cameraPosition;
    vec4 ambientLightColor;
    vec4 lightPositions[maxLights];
    vec4 lightColors[maxLights];
    int numLights;
} ubo;

layout (set = 0, binding = 1) uniform sampler2D texSampler;

layout (push_constant) uniform Push{
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec3 color;
    int isLight;
    int lightIndex;
    float specularExponent;
} push;

void main()
{
    float epsilon = 0.0001;
    vec3 rawColor = texture(texSampler, fragTexCoord).xyz;

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
            vec3 intensity = lightColor.xyz * lightColor.w * attenuation;
            diffuseLight += intensity * cosAngleIncidence;

            vec3 halfAngle = normalize(directionToLight + viewDirection);
            float blinnTerm = dot(surfaceNormal, halfAngle);
            blinnTerm = clamp(blinnTerm, 0, 1);
            blinnTerm = pow(blinnTerm, push.specularExponent);
            specularLight += intensity * blinnTerm;
        }
    }

    vec3 color = diffuseLight * rawColor + specularLight * rawColor;
    FragColor = vec4(color, 1.0);

    if (push.isLight == 1) FragColor += vec4(ubo.lightColors[push.lightIndex].xyz * ubo.lightColors[push.lightIndex].w, 0.0);
}
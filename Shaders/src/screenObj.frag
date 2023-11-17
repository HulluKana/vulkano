#version 460

const int texCount = 10;
const int maxLights = 10;

layout (location = 0) in vec3 fragPos;
layout (location = 1) out vec3 fragNormalWorld;
layout (location = 2) in vec2 fragTexCoord;

layout (location = 0) out vec4 FragColor;

layout (set = 0, binding = 0) uniform GlobalUbo {
    mat4 projectionViewMatrix;
    vec4 cameraPosition;
    vec4 ambientLightColor;
    vec4 lightPositions[maxLights];
    vec4 lightColors[maxLights];
    int numLights;
} ubo;

layout (set = 0, binding = 1) uniform sampler2D texSampler[texCount];

layout (push_constant) uniform Push{
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec3 color;
    int isLight;
    int lightIndex;
    float specularExponent;
    int texIndex;
} push;

void main()
{
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec3 fragNormalWorld;
layout (location = 2) out vec2 fragTexCoord;

const int maxLights = 10;
layout (set = 0, binding = 0) uniform GlobalUbo {
    mat4 projectionViewMatrix;
    vec4 cameraPosition;
    vec4 ambientLightColor;
    vec4 lightPositions[maxLights];
    vec4 lightColors[maxLights];
    int numLights;
} ubo;

layout (push_constant) uniform Push{
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec3 color;
    int isLight;
    int lightIndex;
} push;

void main()
{
    gl_Position = vec4(position, 1.0);
    fragPos = position;
    fragTexCoord = uv;
}
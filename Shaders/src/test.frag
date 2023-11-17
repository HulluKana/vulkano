#version 460

const int texCount = 10;
const int maxLights = 10;

layout (location = 0) in vec3 fragPos;
layout (location = 1) in vec3 fragNormalWorld;
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
    float time;
    float brightness;
    float width;
    float speed;
    float angle;
    int tentacleCount;
} push;

void main()
{
    const float twicePi = 6.283185307;
    const float pi = 3.141592654;
    const float halfPi = 1.570796327;

    float d = length(fragPos.xy);
    float fractD = length(vec2(fract(fragPos.x * 20), fract(fragPos.y * 20)));

    vec2 dir = normalize(fragPos.xy) * -1.0;
    float angle = asin(dir.y);
    if (fragPos.x < 0.0) angle = pi - angle;
    if (fragPos.y > 0.0 && fragPos.x > 0.0) angle += twicePi;

    float phase = sin(d * push.angle + push.time * push.speed * 2.0);
    float fractPhase = sin(fractD * push.angle + push.time * push.speed * 5.0);
    float tentaclePhase = sin(d * push.angle + push.time * push.speed + angle * push.tentacleCount);
    phase = step(1.0 - push.width, phase);
    fractPhase = smoothstep(-push.width, push.width, fractPhase);
    tentaclePhase = step(1.0 - push.width, tentaclePhase);

    FragColor = vec4(tentaclePhase, phase * tentaclePhase, tentaclePhase * fractPhase, 1.0f);
}
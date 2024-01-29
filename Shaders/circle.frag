#version 460

layout (location = 0) in vec3 fragPos;
layout (location = 1) in vec2 fragTexCoord;

layout (location = 0) out vec4 FragColor;

layout (push_constant) uniform Push{
    float x;
    float y;
    float radius;
} push;

void main()
{
    const float aspect = 4.0 / 3.0;
    if (distance(vec2(fragPos.x * aspect, fragPos.y), vec2(push.x * aspect, push.y)) > push.radius) discard;

    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}

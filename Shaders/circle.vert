#version 460

#extension GL_GOOGLE_include_directive : enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec2 fragTexCoord;

layout (push_constant) uniform Push{
    float x;
    float y;
    float radius;
} push;

void main()
{
    const float roomForError = 0.01;
    vec3 adjustedPos = vec3(position.x * (push.radius + roomForError) + push.x, position.y * (push.radius + roomForError) + push.y, position.z);
    gl_Position = vec4(adjustedPos, 1.0f);
    fragPos = adjustedPos.xyz;
    fragTexCoord = uv;
}

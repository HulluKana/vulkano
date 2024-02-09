#version 460

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec2 fragPos;
layout (location = 1) out vec2 fragTexCoord;

void main()
{
    fragPos = position;
    fragTexCoord = uv;
    gl_Position = vec4(position, 1.0f, 1.0f);
}

#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec2 fragTexCoord;

void main()
{
    fragTexCoord = uv;
    gl_Position = vec4(position, 1.0f);
}

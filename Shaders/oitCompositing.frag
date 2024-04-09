#version 460

layout (location = 0) in vec2 fragPos;
layout (location = 1) in vec2 fragTexCoord;

layout (location = 0) out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}

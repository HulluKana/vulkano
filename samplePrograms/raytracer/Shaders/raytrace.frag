#version 460

layout (location = 0) in vec2 fragPos;
layout (location = 1) in vec2 fragTexCoord;

layout (location = 0) out vec4 FragColor;

layout(binding = 0, set = 0, rgba32f) uniform image2D image;

void main()
{
    FragColor = imageLoad(image, ivec2(gl_FragCoord.xy));
}

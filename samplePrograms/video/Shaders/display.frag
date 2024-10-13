#version 460

layout (location = 0) in vec2 fragTexCoord;

layout (location = 0) out vec4 FragColor;

layout(binding = 0, set = 0) uniform sampler2D image;

void main()
{
    FragColor = texture(image, fragTexCoord);
}

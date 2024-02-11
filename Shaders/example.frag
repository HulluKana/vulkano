#version 460

layout (location = 0) in vec2 fragPos;
layout (location = 1) in vec2 fragTexCoord;

layout (location = 0) out vec4 FragColor;

struct Square{
    float x;
    float y;
    float width;
    float height;
    uint seed;
    float r;
    float g;
    float b;
};
layout(set = 0, binding = 0) readonly buffer squareBuffer{Square squares[];};

struct PushData{
    int index;
};
layout (push_constant) uniform Push{PushData push;};

void main()
{
    const Square square = squares[push.index];
    FragColor = vec4(square.r, square.g, square.b, 1.0);
}

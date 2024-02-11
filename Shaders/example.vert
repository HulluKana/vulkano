#version 460

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec2 fragPos;
layout (location = 1) out vec2 fragTexCoord;

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
    Square square = squares[push.index];
    vec2 pos = position;
    if (pos.x > 0.0) pos.x = square.x + square.width / 2.0;
    else pos.x = square.x - square.width / 2.0;
    if (pos.y > 0.0) pos.y = square.y + square.height / 2.0;
    else pos.y = square.y - square.height / 2.0;

    fragPos = pos;
    fragTexCoord = uv;
    gl_Position = vec4(pos, 0.5f, 1.0f);
}

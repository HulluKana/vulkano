#pragma once

#include<stdint.h>

namespace vul{

class settings{

public:
    struct RenderSystemProperties{
        std::string vertShaderName = "../Shaders/bin/default.vert.spv";
        std::string fragShaderName = "../Shaders/bin/default.frag.spv";
    };
    static inline RenderSystemProperties renderSystemProperties{std::string("../Shaders/bin/default.vert.spv"), std::string("../Shaders/bin/default.frag.spv")};

    static inline float maxFps{60.0f};

    struct CameraProperties{
        bool hasPerspective = true;
        float fovY = 80.0f * (M_PI * 2.0f / 360.0f);
        float nearPlane = 0.1f;
        float farPlane = 100.0f;
        float leftPlane = -1.0f;
        float rightPlane = 1.0f;
        float topPlane = -1.0f;
        float bottomPlane = 1.0f;
    };
    static inline CameraProperties cameraProperties{true, 80.0f * (M_PI * 2.0f / 360.0f), 0.1f, 100.0f, -1.0f, 1.0f, -1.0f, 1.0f};

};

}
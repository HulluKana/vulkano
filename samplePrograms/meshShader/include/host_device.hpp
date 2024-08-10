#ifndef HOST_DEVICE
#define HOST_DEVICE

#ifdef __cplusplus

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>

using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
#endif

struct Ubo {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec4 ambientLightColor;
    vec4 cameraPosition;
    vec4 lightPositions[32];
    vec4 lightColors[32];
    uint lightCount;
};

struct PushConstant {
    mat4 modelMatrix;
    uint triangleCount;
    uint vertexOffset;
    uint firstIndex;
    uint matIdx;
};

#endif

#ifndef HOST_DEVICE
#define HOST_DEVICE

#ifdef __cplusplus
#include <stdint.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
using uint = uint32_t;
#endif

#define MAX_LIGHTS 25
#define OIT_LAYERS 8

struct GlobalUbo {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    vec4 cameraPosition; //4th component is ignored

    vec4 ambientLightColor;

    vec4 lightPositions[MAX_LIGHTS]; // 4th component is ignored
    vec4 lightColors[MAX_LIGHTS];
    int numLights;
};

struct DefaultPushConstant{
    mat4 modelMatrix;
    mat4 normalMatrix;
    int matIdx;
};

struct OitPushConstant{
    mat4 modelMatrix;
    mat4 normalMatrix;
    int matIdx;
    uint depthImageIdx;
    float width;
    float height;
};

struct ABuffer {
    uint color;
    uint reflectionColor;
    float depth;
    uint next;
};

#endif

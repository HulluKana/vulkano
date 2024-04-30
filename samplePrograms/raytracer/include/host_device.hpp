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

#define MAX_LIGHTS 10

struct GlobalUbo {
    mat4 inverseProjectionMatrix;
    mat4 inverseViewMatrix;
    vec4 cameraPosition; //4th component is ignored

    vec4 ambientLightColor;

    vec4 lightPositions[MAX_LIGHTS]; // 4th component is ignored
    vec4 lightColors[MAX_LIGHTS];
    int numLights;
};

#endif

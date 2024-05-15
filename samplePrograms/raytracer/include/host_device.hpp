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

#define RESERVOIRS_PER_CELL 128
#define BAD_SAMPLES 16
#define CELL_SIZE 1.0

struct GlobalUbo {
    mat4 inverseProjectionMatrix;
    mat4 inverseViewMatrix;
    vec4 cameraPosition; //4th component is ignored

    vec4 ambientLightColor;

    float pixelSpreadAngle;
    int padding1;
};

struct Reservoir {
    uint lightIdx;
    float averageWeight;
    float targetPdf;
};

#endif

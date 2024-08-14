#ifndef HOST_DEVICE
#define HOST_DEVICE

#ifdef __cplusplus

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>

using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
using i8vec3 = glm::vec<3, int8_t>;
using uvec4 = glm::uvec4;
#endif

#define MAX_MESHLET_VERTICES 64
#define MAX_MESHLET_TRIANGLES 124
#define MESHLETS_PER_TASK_SHADER 32

struct Ubo {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec4 ambientLightColor;
    vec4 cameraPosition;
    vec4 lightPositions[32];
    vec4 lightColors[32];
    uint lightCount;
};

struct MeshInfo {
    mat4 modelMatrix;
    uint meshletOffset;
    uint meshletCount;
    uint matIdx;
    uint padding1;
    uvec4 padding2;
    uvec4 padding3;
    uvec4 padding4;
};

struct Meshlet {
    uint triangleOffset;
    uint vertexOffset;
    uint16_t vertexCount;
    uint16_t triangleCount;
};

struct MeshletBounds {
    vec3 center;
    float radius;
    i8vec3 coneAxis;
    int8_t coneCutoff;
};

#endif

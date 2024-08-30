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
#define VIEWS_PER_SHADOW_MAP 6

struct Ubo {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec4 ambientLightColor;
    vec4 cameraPosition;
    vec4 lightPositions[32];
    vec4 lightColors[32];
    uint lightCount;
};

struct ShadowUbo {
    mat4 viewMatrixes[VIEWS_PER_SHADOW_MAP];
    mat4 projectionMatrix;
    vec4 cameraPosition;
};

struct CubeMapPushConstant {
    mat4 projectionMatrix;
    mat4 originViewMatrix;
};

#endif

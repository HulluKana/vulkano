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
#define LAYERS_IN_SHADOW_MAP 6
#define MAX_POINT_LIGHTS 32
#define MAX_DIRECTIONAL_LIGHTS 8

struct Ubo {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 directionalLightProjViewMats[MAX_DIRECTIONAL_LIGHTS];
    vec4 ambientLightColor;
    vec4 cameraPosition;
    vec4 lightPositions[MAX_POINT_LIGHTS];
    vec4 pointLightColors[MAX_POINT_LIGHTS];
    vec4 lightDirections[MAX_DIRECTIONAL_LIGHTS];
    vec4 directionalLightColors[MAX_DIRECTIONAL_LIGHTS];
    uint pointLightCount;
    uint directionalLightCount;
};

struct ShadowUbo {
    mat4 viewMatrixes[(MAX_POINT_LIGHTS + MAX_DIRECTIONAL_LIGHTS) * LAYERS_IN_SHADOW_MAP];
};

struct ShadowPushConstant {
    mat4 projectionMatrix;
    vec3 cameraPosition;
    uint layerIdx;
    uint viewMatIdx;
};

struct CubeMapPushConstant {
    mat4 projectionMatrix;
    mat4 originViewMatrix;
};

#endif

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

#define VOLUME_LEN 176
#define VOLUME_VOLUME (VOLUME_LEN * VOLUME_LEN * VOLUME_LEN)
#define MESH_PER_CUBE 2
#define CUBES_PER_MESH 16
#define MESH_PER_TASK 32

struct ObjData {
    vec4 pos;
    vec4 color;
};

struct ChunkData {
    vec3 minPos;
    vec3 maxPos;
};

struct RasUbo {
    mat4 viewMatrix;
    mat4 projectionMatrix;
};

struct MeshUbo {
    mat4 viewMatrix;
    mat4 projectionMatrix;
};

struct RtUbo {
    vec4 cameraPosition;
    mat4 inverseViewMatrix;
    mat4 inverseProjectionMatrix;
};

#endif

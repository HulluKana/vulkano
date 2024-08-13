#extension GL_EXT_shader_explicit_arithmetic_types_int16 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable

#include "../include/host_device.hpp"

struct TaskPayload {
    uint meshletIndices[MESHLETS_PER_TASK_SHADER];
    mat4 modelMatrix;
    uint matIdx;
};

const uint meshShadersPerMeshlet = 32;

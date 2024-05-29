#ifndef HOST_DEVICE
#define HOST_DEVICE

#ifdef __cplusplus

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>

using mat4 = glm::mat4;
#endif

struct DefaultPushConstant{
    mat4 modelMatrix;
};

struct Ubo {
    mat4 viewMatrix;
    mat4 projectionMatrix;
};

#endif

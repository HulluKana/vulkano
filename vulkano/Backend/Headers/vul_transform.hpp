#pragma once

#include<glm/gtc/matrix_transform.hpp>

namespace vul{

class transform3D{
    public:
        glm::vec3 pos{};
        glm::vec3 scale{1.0f, 1.0f, 1.0f};
        glm::vec3 rot{};

        glm::mat4 transformMat();
};

};

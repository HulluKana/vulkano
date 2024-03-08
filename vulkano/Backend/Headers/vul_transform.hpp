#pragma once

#include <string>
#include<glm/gtc/matrix_transform.hpp>

namespace vul{

class transform3D{
    public:
        glm::vec3 pos{};
        glm::vec3 scale{1.0f, 1.0f, 1.0f};
        glm::vec3 rot{};

        glm::mat4 transformMat();

        std::string posToStr() const {return std::to_string(pos.x) + " " + std::to_string(pos.y) + " " + std::to_string(pos.z);}
        std::string scaleToStr() const {return std::to_string(scale.x) + " " + std::to_string(scale.y) + " " + std::to_string(scale.z);}
        std::string rotToStr() const {return std::to_string(rot.x) + " " + std::to_string(rot.y) + " " + std::to_string(rot.z);}
};

};

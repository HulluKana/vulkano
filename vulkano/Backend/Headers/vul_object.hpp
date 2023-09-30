#pragma once

#include"vul_model.hpp"

#include<glm/gtc/matrix_transform.hpp>

#include<memory>
#include<unordered_map>

namespace vul{

struct transformComponent{
    glm::vec3 posOffset{};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    glm::vec3 rotation{};

    glm::mat4 transformMat();
    glm::mat3 normalMatrix();
};

class VulObject{
    public:
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t, VulObject>;

        static VulObject createObject(){
            static id_t currentId = 0;
            return VulObject{currentId++};
        }

        const id_t getId() {return id;}

        std::shared_ptr<VulModel> model{};
        glm::vec3 color{};
        transformComponent transform{};
        float specularExponent = 0.0f;

        bool isLight = false;
        glm::vec3 lightColor{0.0f};
        float lightIntensity = 0.0f;

        // Delete the to copy operator from VulObject, you know why
        VulObject(const VulObject &) = delete;
        VulObject &operator=(const VulObject &) = delete;
        // Make the move operator (I think) default, for some reason
        VulObject(VulObject &&) = default;
        VulObject &operator=(VulObject &&) = default;
    private:
        VulObject(id_t objId) : id{objId} {}

        id_t id;
};

}
#pragma once

#include"vul_buffer.hpp"
#include"vul_device.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>

#include<memory>
#include<vector>

namespace vulB {

class VulModel{
    public:
        struct Vertex{
            glm::vec3 pos;
            glm::vec3 normal;
            glm::vec2 uv;

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

            bool operator==(const Vertex &other) const {
                return pos == other.pos && normal == other.normal && uv == other.uv;
            }
        };

        struct Builder {
            std::vector<Vertex> vertices{};
            std::vector<uint32_t> indices{};
            
            void loadModel(const std::string &filepath);
        };

        VulModel(VulDevice &device, const VulModel::Builder &builder);
        ~VulModel();

        /* These 2 lines remove the copy constructor and operator from VulModel class.
        Because I'm using a pointer to stuff and that stuff is initialized by constructor and removed by destructor,
        copying the pointer and then removing the original pointer leaves me with pointer pointing to nothing, possibly leading to VERY nasty bugs */
        VulModel(const VulModel &) = delete;
        VulModel &operator=(const VulModel &) = delete;

        static std::unique_ptr<VulModel> createModelFromFile(VulDevice &device, const std::string &filepath);

        void bind(VkCommandBuffer commandBuffer);
        void draw(VkCommandBuffer commandBuffer);

        uint32_t getTriangleCount() {return indexCount / 3;}

    private:
        void createVertexBuffers(const std::vector<Vertex> &vertices);
        void createIndexBuffers(const std::vector<uint32_t> &indices);

        VulDevice &vulDevice;

        std::unique_ptr<VulBuffer> vertexBuffer;
        uint32_t vertexCount;
        
        bool hasIndexBuffer = false;
        std::unique_ptr<VulBuffer> indexBuffer;
        uint32_t indexCount;

};

}
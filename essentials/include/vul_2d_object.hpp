#pragma once

#include"vul_buffer.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>

namespace vul {
class Object2D{
    public:
        void addSquare(VulDevice &vulDevice, float x, float y, float width, float height);
        void addTriangle(VulDevice &vulDevice, glm::vec2 corner1, glm::vec2 corner2, glm::vec2 corner3);

        void bind(VkCommandBuffer &cmdBuf);
        void draw(VkCommandBuffer &cmdBuf);

        void *pCustomPushData = nullptr;
        uint32_t customPushDataSize = 0;
    private:
        std::unique_ptr<VulBuffer> m_posBuf;
        std::unique_ptr<VulBuffer> m_uvBuf;
        std::unique_ptr<VulBuffer> m_indexBuf;

        uint32_t m_indexCount;
};
}

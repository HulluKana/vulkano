#pragma once

#include"vul_buffer.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>

class Object2D{
    public:
        void addSquare(vulB::VulDevice &vulDevice, float x, float y, float width, float height);
        void addTriangle(vulB::VulDevice &vulDevice, glm::vec2 corner1, glm::vec2 corner2, glm::vec2 corner3);

        void bind(VkCommandBuffer &cmdBuf);
        void draw(VkCommandBuffer &cmdBuf);

        uint32_t renderSystemIndex = 1;
        void *pCustomPushData = nullptr;
        uint32_t customPushDataSize = 0;
    private:
        std::unique_ptr<vulB::VulBuffer> m_posBuf;
        std::unique_ptr<vulB::VulBuffer> m_uvBuf;
        std::unique_ptr<vulB::VulBuffer> m_indexBuf;

        uint32_t m_indexCount;
};

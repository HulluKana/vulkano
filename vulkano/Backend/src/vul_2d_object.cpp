#include "../Headers/vul_2d_object.hpp"
#include <vulkan/vulkan_core.h>

void Object2D::addSquare(vulB::VulDevice &vulDevice, float x, float y, float width, float height)
{
    std::vector<glm::vec2> positions = {
        {x - width, y - height},
        {x - width, y + height},
        {x + width, y + height},
        {x + width, y - height}
    };
    std::vector<glm::vec2> uvCoords = {{0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}};
    std::vector<uint32_t> indices = {0, 1, 2, 0, 3, 2};

    m_posBuf = vulB::VulBuffer::createLocalBufferFromData(vulDevice, positions, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    m_uvBuf = vulB::VulBuffer::createLocalBufferFromData(vulDevice, uvCoords, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    m_indexBuf = vulB::VulBuffer::createLocalBufferFromData(vulDevice, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    m_indexCount = static_cast<uint32_t>(indices.size());
}

void Object2D::addTriangle(vulB::VulDevice &vulDevice, glm::vec2 corner1, glm::vec2 corner2, glm::vec2 corner3)
{
    std::vector<glm::vec2> positions = {corner1, corner2, corner3};
    std::vector<glm::vec2> uvCoords = {{0.0f, 0.0f}, {1.0f, 0.0f}, {0.5f, 1.0f}};
    std::vector<uint32_t> indices = {0, 1, 2};

    m_posBuf = vulB::VulBuffer::createLocalBufferFromData(vulDevice, positions, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    m_uvBuf = vulB::VulBuffer::createLocalBufferFromData(vulDevice, uvCoords, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    m_indexBuf = vulB::VulBuffer::createLocalBufferFromData(vulDevice, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    m_indexCount = static_cast<uint32_t>(indices.size());
}

void Object2D::bind(VkCommandBuffer &cmdBuf)
{
    std::vector<VkBuffer> vertexBuffers = {m_posBuf->getBuffer(), m_uvBuf->getBuffer()};
    std::vector<VkDeviceSize> offsets = {0, 0};

    vkCmdBindVertexBuffers(cmdBuf, 0, static_cast<uint32_t>(vertexBuffers.size()), vertexBuffers.data(), offsets.data());
    vkCmdBindIndexBuffer(cmdBuf, m_indexBuf->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

void Object2D::draw(VkCommandBuffer &cmdBuf)
{
    vkCmdDrawIndexed(cmdBuf, m_indexCount, 1, 0, 0, 0);
}

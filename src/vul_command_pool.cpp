#include <cassert>
#include <cstdint>
#include <iostream>
#include <vul_command_pool.hpp>
#include <vulkan/vulkan_core.h>

namespace vul {

VulCmdPool::VulCmdPool(QueueFamilyType queueFamilyType, VkQueue queue, uint32_t preallocatePrimaryBufferCount, uint32_t preallocateSecondaryBufferCount, const VulDevice &vulDevice) : m_vulDevice(vulDevice)
{
    m_queue = queue;

    VkCommandPoolCreateInfo cmdPoolInfo{};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (queueFamilyType == QueueFamilyType::GraphicsFamily) cmdPoolInfo.queueFamilyIndex = vulDevice.findPhysicalQueueFamilies().graphicsFamily;
    else if (queueFamilyType == QueueFamilyType::ComputeFamily) cmdPoolInfo.queueFamilyIndex = vulDevice.findPhysicalQueueFamilies().computeFamily;
    else if (queueFamilyType == QueueFamilyType::PresentFamily) cmdPoolInfo.queueFamilyIndex = vulDevice.findPhysicalQueueFamilies().presentFamily;

    vkCreateCommandPool(vulDevice.device(), &cmdPoolInfo, nullptr, &m_pool);

    allocateCommandBuffers(VK_COMMAND_BUFFER_LEVEL_PRIMARY, preallocatePrimaryBufferCount);
    allocateCommandBuffers(VK_COMMAND_BUFFER_LEVEL_SECONDARY, preallocateSecondaryBufferCount);
}

VulCmdPool::~VulCmdPool()
{
    for (VkFence fence : m_primaryFences) vkDestroyFence(m_vulDevice.device(), fence, nullptr);
    if (m_pool != VK_NULL_HANDLE) vkDestroyCommandPool(m_vulDevice.device(), m_pool, nullptr);
}

VkCommandBuffer VulCmdPool::getPrimaryCommandBuffer()
{
    VkCommandBufferBeginInfo cmdBufBeginInfo{};
    cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkCommandBuffer cmdBuf;
    bool foundFreeCmdBuf = false;
    for (size_t i = 0; i < m_primaryBuffers.size(); i++) {
        if (!m_primaryStatuses[i] && vkGetFenceStatus(m_vulDevice.device(), m_primaryFences[i]) == VK_SUCCESS) {
            cmdBuf = m_primaryBuffers[i];
            m_primaryStatuses[i] = true;
            foundFreeCmdBuf = true;
            break;
        }
    }
    if (!foundFreeCmdBuf) {
        allocateCommandBuffers(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
        cmdBuf = m_primaryBuffers[m_primaryBuffers.size() - 1];
        m_primaryStatuses[m_primaryStatuses.size() - 1] = true;
    }

    VkResult result = vkBeginCommandBuffer(cmdBuf, &cmdBufBeginInfo);
    assert(result == VK_SUCCESS);
    return cmdBuf;
}

VkCommandBuffer VulCmdPool::getSecondaryCommandBuffer()
{
    VkCommandBufferBeginInfo cmdBufBeginInfo{};
    cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkCommandBuffer cmdBuf;
    bool foundFreeCmdBuf = false;
    for (size_t i = 0; i < m_secondaryBuffers.size(); i++) {
        if (!m_secondaryStatuses[i]) {
            cmdBuf = m_secondaryBuffers[i];
            m_secondaryStatuses[i] = true;
            foundFreeCmdBuf = true;
            break;
        }
    }
    if (!foundFreeCmdBuf) {
        allocateCommandBuffers(VK_COMMAND_BUFFER_LEVEL_SECONDARY, 1);
        cmdBuf = m_secondaryBuffers[m_secondaryBuffers.size() - 1];
        m_secondaryStatuses[m_secondaryStatuses.size() - 1] = true;
    }

    VkResult result = vkBeginCommandBuffer(cmdBuf, &cmdBufBeginInfo);
    assert(result == VK_SUCCESS);
    return cmdBuf;
}

void VulCmdPool::submitAndWait(VkCommandBuffer commandBuffer)
{
    for (size_t i = 0; i < m_primaryBuffers.size(); i++) {
        assert(commandBuffer != m_primaryBuffers[i] || (vkGetFenceStatus(m_vulDevice.device(), m_primaryFences[i]) == VK_SUCCESS && m_primaryStatuses[i]));
        if (commandBuffer == m_primaryBuffers[i]) {
            VkResult result = vkEndCommandBuffer(commandBuffer);
            assert(result == VK_SUCCESS);

            result = vkResetFences(m_vulDevice.device(), 1, &m_primaryFences[i]);
            assert(result == VK_SUCCESS);
            m_primaryStatuses[i] = false;

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;
            result = vkQueueSubmit(m_queue, 1, &submitInfo, m_primaryFences[i]);
            assert(result == VK_SUCCESS);

            result = vkWaitForFences(m_vulDevice.device(), 1, &m_primaryFences[i], VK_TRUE, UINT64_MAX);
            assert(result == VK_SUCCESS);
        }
    }
}

void VulCmdPool::allocateCommandBuffers(VkCommandBufferLevel cmdBufLevel, uint32_t commandBufferCount)
{
    if (commandBufferCount == 0) return;

    VkCommandBufferAllocateInfo cmdBufAllocInfo{};
    cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocInfo.commandPool = m_pool;
    cmdBufAllocInfo.level = cmdBufLevel;
    cmdBufAllocInfo.commandBufferCount = commandBufferCount;

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (cmdBufLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
        m_primaryBuffers.resize(m_primaryBuffers.size() + commandBufferCount);
        VkResult result = vkAllocateCommandBuffers(m_vulDevice.device(), &cmdBufAllocInfo, &m_primaryBuffers[m_primaryBuffers.size() - commandBufferCount]);
        assert(result == VK_SUCCESS);

        m_primaryStatuses.resize(m_primaryBuffers.size());
        m_primaryFences.resize(m_primaryBuffers.size());
        for (size_t i = m_primaryFences.size() - commandBufferCount; i < m_primaryFences.size(); i++) {
            VkResult result = vkCreateFence(m_vulDevice.device(), &fenceCreateInfo, nullptr, &m_primaryFences[i]);
            assert(result == VK_SUCCESS);
        }
    } else {
        m_secondaryBuffers.resize(m_secondaryBuffers.size() + commandBufferCount);
        VkResult result = vkAllocateCommandBuffers(m_vulDevice.device(), &cmdBufAllocInfo, &m_secondaryBuffers[m_secondaryBuffers.size() - commandBufferCount]);
        assert(result == VK_SUCCESS);

        m_secondaryStatuses.resize(m_secondaryBuffers.size());
    }
}

}

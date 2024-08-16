#include <cassert>
#include <cstdint>
#include <iostream>
#include <vul_command_pool.hpp>
#include <vulkan/vulkan_core.h>

namespace vul {

VulCmdPool::VulCmdPool(QueueType queueType, uint32_t preallocatePrimaryBufferCount, uint32_t preallocateSecondaryBufferCount, const VulDevice &vulDevice, uint32_t sideQueueIndex) : m_vulDevice(vulDevice)
{
    VkCommandPoolCreateInfo cmdPoolInfo{};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (queueType == QueueType::main) {
        cmdPoolInfo.queueFamilyIndex = vulDevice.getQueueFamilies().mainFamily;
        m_queue = vulDevice.mainQueue();
    } else if (queueType == QueueType::compute) {
        cmdPoolInfo.queueFamilyIndex = vulDevice.getQueueFamilies().computeFamily;
        m_queue = vulDevice.computeQueue();
    } else if (queueType == QueueType::transfer) {
        cmdPoolInfo.queueFamilyIndex = vulDevice.getQueueFamilies().transferFamily;
        m_queue = vulDevice.transferQueue();
    } else if (queueType == QueueType::side) {
        cmdPoolInfo.queueFamilyIndex = vulDevice.getQueueFamilies().mainFamily;
        m_queue = vulDevice.sideQueues()[sideQueueIndex];
    }

    vkCreateCommandPool(vulDevice.device(), &cmdPoolInfo, nullptr, &m_pool);

    allocateCommandBuffers(VK_COMMAND_BUFFER_LEVEL_PRIMARY, preallocatePrimaryBufferCount);
    allocateCommandBuffers(VK_COMMAND_BUFFER_LEVEL_SECONDARY, preallocateSecondaryBufferCount);
}

VulCmdPool::~VulCmdPool()
{
    for (VkFence fence : m_fences) vkDestroyFence(m_vulDevice.device(), fence, nullptr);
    for (VkSemaphore semaphore : m_semaphores) vkDestroySemaphore(m_vulDevice.device(), semaphore, nullptr);
    if (m_pool != VK_NULL_HANDLE) vkDestroyCommandPool(m_vulDevice.device(), m_pool, nullptr);
}

VkCommandBuffer VulCmdPool::getPrimaryCommandBuffer()
{
    VkCommandBufferBeginInfo cmdBufBeginInfo{};
    cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkCommandBuffer cmdBuf;
    bool foundFreeCmdBuf = false;
    for (size_t i = 0; i < m_primaryBuffers.size(); i++) {
        if (!m_primaryStatuses[i] && vkGetFenceStatus(m_vulDevice.device(), m_fences[i]) == VK_SUCCESS) {
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
    VkCommandBufferInheritanceInfo inheritanceInfo{};
    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

    VkCommandBufferBeginInfo cmdBufBeginInfo{};
    cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmdBufBeginInfo.pInheritanceInfo = &inheritanceInfo;

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

void VulCmdPool::submit(VkCommandBuffer commandBuffer, bool wait)
{
    for (size_t i = 0; i < m_primaryBuffers.size(); i++) {
        assert(commandBuffer != m_primaryBuffers[i] || (vkGetFenceStatus(m_vulDevice.device(), m_fences[i]) == VK_SUCCESS && m_primaryStatuses[i]));
        if (commandBuffer == m_primaryBuffers[i]) {
            VkResult result = vkEndCommandBuffer(commandBuffer);
            assert(result == VK_SUCCESS);

            result = vkResetFences(m_vulDevice.device(), 1, &m_fences[i]);
            assert(result == VK_SUCCESS);
            m_primaryStatuses[i] = false;

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;
            result = vkQueueSubmit(m_queue, 1, &submitInfo, m_fences[i]);
            assert(result == VK_SUCCESS);

            if (wait) {
                result = vkWaitForFences(m_vulDevice.device(), 1, &m_fences[i], VK_TRUE, UINT64_MAX);
                assert(result == VK_SUCCESS);
            }

            return;
        }
    }
}

VkSemaphore VulCmdPool::submitAndSynchronize(VkCommandBuffer commandBuffer, VkSemaphore waitSemaphore, bool returnSignaledSemaphore, bool wait)
{
    for (size_t i = 0; i < m_primaryBuffers.size(); i++) {
        assert(commandBuffer != m_primaryBuffers[i] || (vkGetFenceStatus(m_vulDevice.device(), m_fences[i]) == VK_SUCCESS && m_primaryStatuses[i]));
        if (commandBuffer == m_primaryBuffers[i]) {
            VkResult result = vkEndCommandBuffer(commandBuffer);
            assert(result == VK_SUCCESS);

            result = vkResetFences(m_vulDevice.device(), 1, &m_fences[i]);
            assert(result == VK_SUCCESS);
            m_primaryStatuses[i] = false;

            VkSemaphore signaledSemaphore = VK_NULL_HANDLE;
            if (returnSignaledSemaphore) {
                if (m_cmdBufToSemaphoresMap[i] < 0) {
                    VkSemaphoreCreateInfo semaphoreCreateInfo{};
                    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                    vkCreateSemaphore(m_vulDevice.device(), &semaphoreCreateInfo, nullptr, &signaledSemaphore);
                    m_cmdBufToSemaphoresMap[i] = m_semaphores.size();
                    m_semaphores.push_back(signaledSemaphore);
                } else signaledSemaphore = m_semaphores[m_cmdBufToSemaphoresMap[i]];
            }

            VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;
            if (waitSemaphore != VK_NULL_HANDLE) {
                submitInfo.pWaitSemaphores = &waitSemaphore;
                submitInfo.pWaitDstStageMask = &waitStageMask;
                submitInfo.waitSemaphoreCount = 1;
            }
            if (returnSignaledSemaphore) {
                submitInfo.pSignalSemaphores = &signaledSemaphore;
                submitInfo.signalSemaphoreCount = 1;
            }
            result = vkQueueSubmit(m_queue, 1, &submitInfo, m_fences[i]);
            assert(result == VK_SUCCESS);

            if (wait) {
                result = vkWaitForFences(m_vulDevice.device(), 1, &m_fences[i], VK_TRUE, UINT64_MAX);
                assert(result == VK_SUCCESS);
            }

            return signaledSemaphore;
        }
    }
    return VK_NULL_HANDLE;
}

void VulCmdPool::endCommandBuffer(VkCommandBuffer commandBuffer)
{
    for (size_t i = 0; i < m_secondaryBuffers.size(); i++) {
        assert(commandBuffer != m_secondaryBuffers[i] || m_secondaryStatuses[i]);
        if (commandBuffer == m_secondaryBuffers[i]) {
            VkResult result = vkEndCommandBuffer(commandBuffer);
            assert(result == VK_SUCCESS);
            m_secondaryStatuses[i] = false;
            return;
        }
    }
    for (size_t i = 0; i < m_primaryBuffers.size(); i++) {
        assert(commandBuffer != m_primaryBuffers[i] || m_primaryStatuses[i]);
        if (commandBuffer == m_primaryBuffers[i]) {
            VkResult result = vkEndCommandBuffer(commandBuffer);
            assert(result == VK_SUCCESS);
            m_primaryStatuses[i] = false;
            return;
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
        m_fences.resize(m_primaryBuffers.size());
        for (size_t i = m_fences.size() - commandBufferCount; i < m_fences.size(); i++) {
            VkResult result = vkCreateFence(m_vulDevice.device(), &fenceCreateInfo, nullptr, &m_fences[i]);
            assert(result == VK_SUCCESS);
        }
        m_cmdBufToSemaphoresMap.resize(m_primaryBuffers.size());
        for (size_t i = m_cmdBufToSemaphoresMap.size() - commandBufferCount; i < m_cmdBufToSemaphoresMap.size(); i++)
            m_cmdBufToSemaphoresMap[i] = -1;
    } else {
        m_secondaryBuffers.resize(m_secondaryBuffers.size() + commandBufferCount);
        VkResult result = vkAllocateCommandBuffers(m_vulDevice.device(), &cmdBufAllocInfo, &m_secondaryBuffers[m_secondaryBuffers.size() - commandBufferCount]);
        assert(result == VK_SUCCESS);

        m_secondaryStatuses.resize(m_secondaryBuffers.size());
    }
}

}

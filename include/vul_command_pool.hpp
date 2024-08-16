#pragma once

#include "vul_device.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>
namespace vul {

class VulCmdPool {
    public:
        enum class QueueType {
            main,
            compute,
            transfer,
            side
        };

        VulCmdPool(QueueType queueType, uint32_t preallocatePrimaryBufferCount, uint32_t preallocateSecondaryBufferCount, const VulDevice &vulDevice, uint32_t sideQueueIndex = 0);
        ~VulCmdPool();

        VulCmdPool(const VulCmdPool &) = delete;
        VulCmdPool &operator=(const VulCmdPool &) = delete;
        VulCmdPool(VulCmdPool &&) = delete;
        VulCmdPool &operator=(VulCmdPool &&) = delete;

        VkCommandBuffer getPrimaryCommandBuffer();
        VkCommandBuffer getSecondaryCommandBuffer();

        void submit(VkCommandBuffer commandBuffer, bool wait);
        VkSemaphore submitAndSynchronize(VkCommandBuffer commandBuffer, VkSemaphore waitSemaphore, bool returnSignaledSemaphore, bool wait);
        void endCommandBuffer(VkCommandBuffer commandBuffer);

        VkCommandPool getPool() const {return m_pool;}

    private:
        void allocateCommandBuffers(VkCommandBufferLevel cmdBufLevel, uint32_t commandBufferCount);

        std::vector<bool> m_primaryStatuses;
        std::vector<bool> m_secondaryStatuses;
        std::vector<VkCommandBuffer> m_primaryBuffers;
        std::vector<VkCommandBuffer> m_secondaryBuffers;
        std::vector<VkFence> m_fences;
        std::vector<int> m_cmdBufToSemaphoresMap;
        std::vector<VkSemaphore> m_semaphores;
        VkCommandPool m_pool;
        VkQueue m_queue;

        const VulDevice &m_vulDevice;
};

}

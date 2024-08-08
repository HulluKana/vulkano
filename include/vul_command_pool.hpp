#pragma once

#include "vul_device.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>
namespace vul {

class VulCmdPool {
    public:
        enum class QueueFamilyType {
            GraphicsFamily,
            ComputeFamily,
            PresentFamily
        };

        VulCmdPool(QueueFamilyType queueFamilyType, VkQueue queue, uint32_t preallocatePrimaryBufferCount, uint32_t preallocateSecondaryBufferCount, const VulDevice &vulDevice);
        ~VulCmdPool();

        VulCmdPool(const VulCmdPool &) = delete;
        VulCmdPool &operator=(const VulCmdPool &) = delete;
        VulCmdPool(VulCmdPool &&) = delete;
        VulCmdPool &operator=(VulCmdPool &&) = delete;

        VkCommandBuffer getPrimaryCommandBuffer();
        VkCommandBuffer getSecondaryCommandBuffer();
        void submitAndWait(VkCommandBuffer commandBuffer);

        VkCommandPool getPool() const {return m_pool;}

    private:
        void allocateCommandBuffers(VkCommandBufferLevel cmdBufLevel, uint32_t commandBufferCount);

        std::vector<bool> m_primaryStatuses;
        std::vector<bool> m_secondaryStatuses;
        std::vector<VkCommandBuffer> m_primaryBuffers;
        std::vector<VkCommandBuffer> m_secondaryBuffers;
        std::vector<VkFence> m_primaryFences;
        VkCommandPool m_pool;
        VkQueue m_queue;

        const VulDevice &m_vulDevice;
};

}

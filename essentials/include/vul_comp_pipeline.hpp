#pragma once

#include"vul_device.hpp"
#include <vulkan/vulkan_core.h>

namespace vul
{

class VulCompPipeline
{
    public:
        VulCompPipeline(const std::vector<std::string> &shaderNames, const std::vector<VkDescriptorSetLayout> &setLayouts, vulB::VulDevice &device, uint32_t maxFramesInFlight);
        ~VulCompPipeline();

        VulCompPipeline(const VulCompPipeline &) = delete;
        VulCompPipeline &operator=(const VulCompPipeline &) = delete;
        VulCompPipeline(VulCompPipeline &&) = default;
        VulCompPipeline &operator=(VulCompPipeline &&) = default;

        void begin(const std::vector<VkDescriptorSet> &sets);
        void dispatchAll(uint32_t x, uint32_t y, uint32_t z);
        void dispatchOne(uint32_t index, uint32_t x, uint32_t y, uint32_t z);
        void end(bool waitForSubmitToFinish);

        void *pPushData = nullptr;
        uint32_t pushSize = 0;

    private:
        std::vector<VkPipeline> m_pipelines;
        VkPipelineLayout m_layout;

        std::vector<VkCommandBuffer> m_cmdBufs;
        std::vector<VkFence> m_fences;
        uint32_t m_frame = 0;
        uint32_t m_maxFramesInFlight;

        vulB::VulDevice &m_vulDevice;
};

}

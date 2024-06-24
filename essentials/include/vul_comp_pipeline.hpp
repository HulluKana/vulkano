#pragma once

#include"vul_device.hpp"
#include <vulkan/vulkan_core.h>

namespace vul
{

class VulCompPipeline
{
    public:
        VulCompPipeline(const std::string &shaderName, const std::vector<VkDescriptorSetLayout> &setLayouts, VulDevice &device, uint32_t maxFramesInFlight);
        ~VulCompPipeline();

        VulCompPipeline(const VulCompPipeline &) = delete;
        VulCompPipeline &operator=(const VulCompPipeline &) = delete;
        VulCompPipeline(VulCompPipeline &&) = default;
        VulCompPipeline &operator=(VulCompPipeline &&) = default;

        void begin(const std::vector<VkDescriptorSet> &sets);
        void dispatch(uint32_t x, uint32_t y, uint32_t z);
        void end(bool waitForSubmitToFinish);

        void *pPushData = nullptr;
        uint32_t pushSize = 0;

    private:
        VkPipeline m_pipeline;
        VkPipelineLayout m_layout;

        std::vector<VkCommandBuffer> m_cmdBufs;
        std::vector<VkFence> m_fences;
        uint32_t m_frame = 0;
        uint32_t m_maxFramesInFlight;

        VulDevice &m_vulDevice;
};

}

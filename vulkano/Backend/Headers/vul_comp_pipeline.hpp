#pragma once

#include"vul_device.hpp"
#include <vulkan/vulkan_core.h>

namespace vulB
{

class VulCompPipeline
{
    public:
        VulCompPipeline(const std::string &shaderName, const std::vector<VkDescriptorSetLayout> &setLayouts, VulDevice &device);
        ~VulCompPipeline();

        VulCompPipeline(const VulCompPipeline &) = delete;
        VulCompPipeline &operator=(const VulCompPipeline &) = delete;

        void dispatch(uint32_t x, uint32_t y, uint32_t z, const std::vector<VkDescriptorSet> &sets);

        void *pPushData = nullptr;
        uint32_t pushSize = 0;

    private:
        VkPipeline m_pipeline;
        VkPipelineLayout m_layout;
        VkShaderModule m_shader;

        std::vector<VkCommandBuffer> m_cmdBufs;
        std::vector<VkFence> m_fences;
        int m_frame = 0;

        VulDevice &m_vulDevice;
};

}

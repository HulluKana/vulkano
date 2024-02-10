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

        void dispatch(uint32_t x, uint32_t y, uint32_t z, const std::vector<VkDescriptorSet> &sets, VkCommandBuffer cmdBuf);

    private:
        VkPipeline m_pipeline;
        VkPipelineLayout m_layout;
        VkShaderModule m_shader;

        VulDevice &m_vulDevice;
};

}

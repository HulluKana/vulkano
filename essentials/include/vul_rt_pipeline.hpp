#pragma once

#include "vul_device.hpp"
#include <vul_pipeline.hpp>
#include <vulkan/vulkan_core.h>

namespace vul {

class VulRtPipeline {
    public:
        VulRtPipeline(vulB::VulDevice &vulDevice, const std::vector<std::string> &raygenShaders, const std::vector<std::string> &missShaders,
                const std::vector<std::string> &closestHitShaders, const std::vector<VkDescriptorSetLayout> &setLayouts);
        ~VulRtPipeline();

        VulRtPipeline(const VulRtPipeline &) = delete;
        VulRtPipeline &operator=(const VulRtPipeline &) = delete;
        VulRtPipeline(VulRtPipeline &&) = default;
    private:
        void createPipeline(const std::vector<std::string> &raygenShaders, const std::vector<std::string> &missShaders,
                const std::vector<std::string> &closestHitShaders, const std::vector<VkDescriptorSetLayout> &setLayouts);

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_shaderGroups;
        VkPipelineLayout m_layout;
        VkPipeline m_pipeline;

        vulB::VulDevice &m_vulDevice;
};

}

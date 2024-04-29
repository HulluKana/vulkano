#pragma once

#include "vul_buffer.hpp"
#include "vul_device.hpp"
#include <vul_pipeline.hpp>
#include <vulkan/vulkan_core.h>

namespace vul {

class VulRtPipeline {
    public:
        VulRtPipeline(vulB::VulDevice &vulDevice, const std::string &raygenShader, const std::vector<std::string> &missShaders,
                const std::vector<std::string> &closestHitShaders, const std::vector<VkDescriptorSetLayout> &setLayouts);
        ~VulRtPipeline();

        VulRtPipeline(const VulRtPipeline &) = delete;
        VulRtPipeline &operator=(const VulRtPipeline &) = delete;
        VulRtPipeline(VulRtPipeline &&) = default;
    private:
        void createPipeline(const std::string &raygenShader, const std::vector<std::string> &missShaders,
                const std::vector<std::string> &closestHitShaders, const std::vector<VkDescriptorSetLayout> &setLayouts);
        void createSBT(uint32_t missCount, uint32_t hitCount);

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_shaderGroups;
        VkPipelineLayout m_layout;
        VkPipeline m_pipeline;

        std::unique_ptr<vulB::VulBuffer> m_SBTBuffer;
        VkStridedDeviceAddressRegionKHR m_rgenRegion{};
        VkStridedDeviceAddressRegionKHR m_rmissRegion{};
        VkStridedDeviceAddressRegionKHR m_rhitRegion{};
        VkStridedDeviceAddressRegionKHR m_callRegion{};

        vulB::VulDevice &m_vulDevice;
};

}

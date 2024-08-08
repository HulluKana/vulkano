#pragma once

#include "vul_command_pool.hpp"
#include <vul_buffer.hpp>
#include <vulkan/vulkan_core.h>

namespace vul {

class VulRtPipeline {
    public:
        struct HitGroup {
            int closestHitIdx;
            int anyHitIdx;
            int intersectionIdx;
        };
        VulRtPipeline(const VulDevice &vulDevice, const std::string &raygenShader, const std::vector<std::string> &missShaders,
                const std::vector<std::string> &closestHitShaders, const std::vector<std::string> &anyHitShaders,
                const std::vector<std::string> &intersectionShaders, const std::vector<HitGroup> &hitGroups,
                const std::vector<VkDescriptorSetLayout> &setLayouts, VulCmdPool &cmdPool);
        ~VulRtPipeline();

        VulRtPipeline(const VulRtPipeline &) = delete;
        VulRtPipeline &operator=(const VulRtPipeline &) = delete;
        VulRtPipeline(VulRtPipeline &&) = default;

        void traceRays(uint32_t width, uint32_t height, uint32_t pushConstantSize, void *pushConstant, const std::vector<VkDescriptorSet> &descSets,
                VkCommandBuffer cmdBuf);
    private:
        void createPipeline(const std::string &raygenShader, const std::vector<std::string> &missShaders,
                const std::vector<std::string> &closestHitShaders, const std::vector<std::string> &anyHitShaders,
                const std::vector<std::string> &intersectionShaders, const std::vector<HitGroup> &hitGroups,
                const std::vector<VkDescriptorSetLayout> &setLayouts);
        void createSBT(uint32_t missCount, uint32_t hitCount, VulCmdPool &cmdPool);

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_shaderGroups;
        VkPipelineLayout m_layout;
        VkPipeline m_pipeline;

        std::unique_ptr<VulBuffer> m_SBTBuffer;
        VkStridedDeviceAddressRegionKHR m_rgenRegion{};
        VkStridedDeviceAddressRegionKHR m_rmissRegion{};
        VkStridedDeviceAddressRegionKHR m_rhitRegion{};
        VkStridedDeviceAddressRegionKHR m_callRegion{};

        const VulDevice &m_vulDevice;
};

}

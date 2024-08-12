#pragma once

#include <vul_device.hpp>
#include <vulkan/vulkan_core.h>

namespace vul {

class VulMeshPipeline {
    public:
        struct PipelineConfigInfo {
            std::vector<VkDescriptorSetLayout> setLayouts;
            std::vector<VkFormat> colorAttachmentFormats;
            VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED;
            VkCullModeFlagBits cullMode = VK_CULL_MODE_NONE;
            bool enableColorBlending = false;
            VkBlendOp blendOp = VK_BLEND_OP_ADD;
            VkBlendFactor blendSrcFactor = VK_BLEND_FACTOR_ONE;
            VkBlendFactor blendDstFactor = VK_BLEND_FACTOR_ZERO;
            VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
            float lineWidth = 1.0f;
            VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        };

        VulMeshPipeline(const VulDevice &vulDevice, const std::string &taskShaderFile, const std::string &meshShaderFile, const std::string &fragShaderFile, const PipelineConfigInfo &configInfo);
        ~VulMeshPipeline();

        VulMeshPipeline(const VulMeshPipeline &) = delete;
        VulMeshPipeline &operator=(const VulMeshPipeline &) = delete;
        VulMeshPipeline(VulMeshPipeline &&) = default;

        void meshShade(uint32_t x, uint32_t y, uint32_t z, const void *pushData, uint32_t pushDataSize, const std::vector<VkDescriptorSet> &descSets, VkCommandBuffer cmdBuf);
        void meshShadeIndirect(VkBuffer indirectBuffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride,
                const void *pushData, uint32_t pushDataSize, const std::vector<VkDescriptorSet> &descSets, VkCommandBuffer cmdBuf);

    private:
        VkPipeline m_pipeline;
        VkPipelineLayout m_layout;
        const VulDevice &m_vulDevice;
};

}

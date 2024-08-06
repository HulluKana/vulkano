#pragma once

#include"vul_device.hpp"
#include<string>
#include<vector>
#include<memory>
#include <vulkan/vulkan_core.h>

namespace vul{

class VulPipeline{
    public:
        struct PipelineConfigInfo {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
            std::vector<VkVertexInputBindingDescription> bindingDescriptions;
            std::vector<VkDescriptorSetLayout> setLayouts;
            std::vector<VkFormat> colorAttachmentFormats;
            VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED;
            VkCullModeFlagBits cullMode = VK_CULL_MODE_NONE;
            bool enableColorBlending = false;
            VkBlendOp blendOp = VK_BLEND_OP_ADD;
            VkBlendFactor blendSrcFactor = VK_BLEND_FACTOR_ONE;
            VkBlendFactor blendDstFactor = VK_BLEND_FACTOR_ZERO;
            VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
            float lineWidth = 1.0f;
        };

        VulPipeline(const VulDevice& device, const std::string& vertFile, const std::string& fragFile, const PipelineConfigInfo& configInfo);
        ~VulPipeline();

        /* These 2 lines remove the copy constructor and operator from VulPipeline class.
        Because I'm using a pointer to some stuff and that stuff is initialized by constructor and removed by destructor,
        copying the pointer and then removing the original pointer leaves me with pointer pointing to nothing, possibly leading to VERY nasty bugs */
        VulPipeline(const VulPipeline &) = delete;
        VulPipeline &operator=(const VulPipeline &) = delete;

        struct DrawData {
            uint32_t indexCount = 0;
            uint32_t firstIndex = 0;
            int32_t vertexOffset = 0;
            uint32_t instanceCount = 1;
            uint32_t firstInstance = 0;
            std::shared_ptr<void> pPushData = nullptr;
            uint32_t pushDataSize = 0;
        };
        void draw(  VkCommandBuffer cmdBuf, const std::vector<VkDescriptorSet> &descriptorSets, const std::vector<VkBuffer> &vertexBuffers,
                    VkBuffer indexBuffer, const std::vector<DrawData> &drawDatas);

        struct PipelineContents {
            VkPipeline pipeline;
            VkPipelineLayout layout;
        };
        static PipelineContents createPipelineContents(const VulDevice &vulDevice, const std::vector<VkPipelineShaderStageCreateInfo> &shaderStageCreateInfos,
            const std::vector<VkVertexInputAttributeDescription> &attributeDescriptions, const std::vector<VkVertexInputBindingDescription> &bindingDescriptions,
            const std::vector<VkDescriptorSetLayout> &setLayouts, const std::vector<VkFormat> &colorAttachmentFormats, VkFormat depthAttachmentFormat,
            VkCullModeFlagBits cullMode, bool enableColorBlending, VkBlendOp blendOp, VkBlendFactor blendSrcFactor, VkBlendFactor blendDstFactor,
            VkPolygonMode polygonMode, float lineWidth, VkPrimitiveTopology primitiveTopology);
        static VkShaderModule createShaderModule(const VulDevice &vulDevice, const std::string &filePath);

        VkPipeline getPipeline() const {return m_pipeline;}
        VkPipelineLayout getPipelineLayout() const {return m_layout;}
    private:
        const VulDevice& m_vulDevice;
        VkPipeline m_pipeline;
        VkPipelineLayout m_layout;
};
}

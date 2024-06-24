#include "vul_debug_tools.hpp"
#include "vul_pipeline.hpp"
#include <vector>
#include <vul_mesh_pipeline.hpp>
#include <vulkan/vulkan_core.h>

namespace vul {

VulMeshPipeline::VulMeshPipeline(const VulDevice &vulDevice, const std::string &meshShaderFile, const std::string &fragShaderFile, const PipelineConfigInfo &configInfo)
    : m_vulDevice{vulDevice}
{
    VkShaderModule meshShader = VulPipeline::createShaderModule(vulDevice, meshShaderFile);
    VkShaderModule fragShader = VulPipeline::createShaderModule(vulDevice, fragShaderFile);

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages(2);
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_MESH_BIT_EXT;
    shaderStages[0].module = meshShader;
    shaderStages[0].pName = "main";
    shaderStages[0].flags = 0;
    shaderStages[0].pNext = nullptr;
    shaderStages[0].pSpecializationInfo = nullptr;

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShader;
    shaderStages[1].pName = "main";
    shaderStages[1].flags = 0;
    shaderStages[1].pNext = nullptr;
    shaderStages[1].pSpecializationInfo = nullptr;

    VulPipeline::PipelineContents pipelineContents = VulPipeline::createPipelineContents(vulDevice, shaderStages, {}, {}, configInfo.setLayouts,
            configInfo.colorAttachmentFormats, configInfo.depthAttachmentFormat, configInfo.cullMode, configInfo.enableColorBlending, configInfo.blendOp,
            configInfo.blendSrcFactor, configInfo.blendDstFactor);

    m_pipeline = pipelineContents.pipeline;
    m_layout = pipelineContents.layout;

    vkDestroyShaderModule(vulDevice.device(), meshShader, nullptr);
    vkDestroyShaderModule(vulDevice.device(), fragShader, nullptr);

    VUL_NAME_VK_MANUAL(m_pipeline, "meshPipeline");
    VUL_NAME_VK_MANUAL(m_layout, "meshPipelineLayout");
}

VulMeshPipeline::~VulMeshPipeline()
{
    vkDestroyPipelineLayout(m_vulDevice.device(), m_layout, nullptr);
    vkDestroyPipeline(m_vulDevice.device(), m_pipeline, nullptr);
}

void VulMeshPipeline::meshShade(uint32_t x, uint32_t y, uint32_t z, VkCommandBuffer cmdBuf)
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdDrawMeshTasksEXT(cmdBuf, x, y, z);
}

}

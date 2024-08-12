#include "vul_debug_tools.hpp"
#include "vul_pipeline.hpp"
#include <vector>
#include <vul_mesh_pipeline.hpp>
#include <vulkan/vulkan_core.h>

namespace vul {

VulMeshPipeline::VulMeshPipeline(const VulDevice &vulDevice, const std::string &taskShaderFile, const std::string &meshShaderFile, const std::string &fragShaderFile, const PipelineConfigInfo &configInfo)
    : m_vulDevice{vulDevice}
{
    VkShaderModule taskShader = VK_NULL_HANDLE;
    if (taskShaderFile.length() > 0) taskShader = VulPipeline::createShaderModule(vulDevice, taskShaderFile);
    VkShaderModule meshShader = VulPipeline::createShaderModule(vulDevice, meshShaderFile);
    VkShaderModule fragShader = VulPipeline::createShaderModule(vulDevice, fragShaderFile);

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages(2 + (taskShader != VK_NULL_HANDLE));
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

    if (taskShader != VK_NULL_HANDLE) {
        shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[2].stage = VK_SHADER_STAGE_TASK_BIT_EXT;
        shaderStages[2].module = taskShader;
        shaderStages[2].pName = "main";
        shaderStages[2].flags = 0;
        shaderStages[2].pNext = nullptr;
        shaderStages[2].pSpecializationInfo = nullptr;
    }

    VulPipeline::PipelineContents pipelineContents = VulPipeline::createPipelineContents(vulDevice, shaderStages, {}, {}, configInfo.setLayouts,
            configInfo.colorAttachmentFormats, configInfo.depthAttachmentFormat, configInfo.cullMode, configInfo.enableColorBlending, configInfo.blendOp,
            configInfo.blendSrcFactor, configInfo.blendDstFactor, configInfo.polygonMode, configInfo.lineWidth, configInfo.primitiveTopology, true);

    m_pipeline = pipelineContents.pipeline;
    m_layout = pipelineContents.layout;

    if (VK_NULL_HANDLE != taskShader) vkDestroyShaderModule(vulDevice.device(), taskShader, nullptr);
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

void VulMeshPipeline::meshShade(uint32_t x, uint32_t y, uint32_t z, const void *pushData, uint32_t pushDataSize, const std::vector<VkDescriptorSet> &descSets, VkCommandBuffer cmdBuf)
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    if (descSets.size() > 0) vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_layout, 0, static_cast<uint32_t>(descSets.size()), descSets.data(), 0, nullptr);
    if (pushDataSize > 0) vkCmdPushConstants(cmdBuf, m_layout, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, pushDataSize, pushData);
    vkCmdDrawMeshTasksEXT(cmdBuf, x, y, z);
}

void VulMeshPipeline::meshShadeIndirect(VkBuffer indirectBuffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride,
        const void *pushData, uint32_t pushDataSize, const std::vector<VkDescriptorSet> &descSets, VkCommandBuffer cmdBuf)
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    if (descSets.size() > 0) vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_layout, 0, static_cast<uint32_t>(descSets.size()), descSets.data(), 0, nullptr);
    if (pushDataSize > 0) vkCmdPushConstants(cmdBuf, m_layout, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, pushDataSize, pushData);
    vkCmdDrawMeshTasksIndirectEXT(cmdBuf, indirectBuffer, offset, drawCount, stride);
}

}

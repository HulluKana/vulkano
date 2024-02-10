#include"../Headers/vul_comp_pipeline.hpp"
#include"../Headers/vul_pipeline.hpp"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace vulB
{

VulCompPipeline::VulCompPipeline(const std::string &shaderName, const std::vector<VkDescriptorSetLayout> &setLayouts, VulDevice &device) : m_vulDevice{device}
{
    std::vector<char> shaderCode = VulPipeline::readFile(shaderName);
    VulPipeline::createShaderModule(m_vulDevice, shaderCode, &m_shader);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = m_shader;
    stageInfo.pName = "main";

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = setLayouts.size();
    layoutInfo.pSetLayouts = setLayouts.data();
    if (vkCreatePipelineLayout(m_vulDevice.device(), &layoutInfo, nullptr, &m_layout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute pipeline layout");
    
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = m_layout;
    pipelineInfo.stage = stageInfo;
    if (vkCreateComputePipelines(m_vulDevice.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute pipeline");
}

VulCompPipeline::~VulCompPipeline()
{
    vkDestroyPipeline(m_vulDevice.device(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_vulDevice.device(), m_layout, nullptr);
    vkDestroyShaderModule(m_vulDevice.device(), m_shader, nullptr);
}

void VulCompPipeline::dispatch(uint32_t x, uint32_t y, uint32_t z, const std::vector<VkDescriptorSet> &sets, VkCommandBuffer cmdBuf)
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_layout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
    vkCmdDispatch(cmdBuf, x, y, z);
}

}

#include"../Headers/vul_comp_pipeline.hpp"
#include"../Headers/vul_pipeline.hpp"
#include"../Headers/vul_swap_chain.hpp"
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

    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstant.size = m_vulDevice.properties.limits.maxPushConstantsSize;
    pushConstant.offset = 0;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = setLayouts.size();
    layoutInfo.pSetLayouts = setLayouts.data();
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstant;
    if (vkCreatePipelineLayout(m_vulDevice.device(), &layoutInfo, nullptr, &m_layout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute pipeline layout");
    
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = m_layout;
    pipelineInfo.stage = stageInfo;
    if (vkCreateComputePipelines(m_vulDevice.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute pipeline");

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_vulDevice.getCommandPool();
    allocInfo.commandBufferCount = 2;

    m_cmdBufs.resize(VulSwapChain::MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateCommandBuffers(m_vulDevice.device(), &allocInfo, m_cmdBufs.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffer while creating compute pipeline");


    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    m_fences.resize(VulSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
        if (vkCreateFence(m_vulDevice.device(), &fenceInfo, nullptr, &m_fences[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create fence while creating compute pipeline");
    }
}

VulCompPipeline::~VulCompPipeline()
{
    vkDestroyPipeline(m_vulDevice.device(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_vulDevice.device(), m_layout, nullptr);
    vkDestroyShaderModule(m_vulDevice.device(), m_shader, nullptr);
    vkFreeCommandBuffers(m_vulDevice.device(), m_vulDevice.getCommandPool(), 2, m_cmdBufs.data());
    for (int i = 0; i < VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
        vkDestroyFence(m_vulDevice.device(), m_fences[i], nullptr);
}

void VulCompPipeline::dispatch(uint32_t x, uint32_t y, uint32_t z, const std::vector<VkDescriptorSet> &sets)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkWaitForFences(m_vulDevice.device(), 1, &m_fences[m_frame], VK_TRUE, UINT64_MAX);
    if (vkBeginCommandBuffer(m_cmdBufs[m_frame], &beginInfo) != VK_SUCCESS){
        throw std::runtime_error("Failed to begin recording command buffer");
    }
    vkResetFences(m_vulDevice.device(), 1, &m_fences[m_frame]);

    vkCmdBindPipeline(m_cmdBufs[m_frame], VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
    vkCmdBindDescriptorSets(m_cmdBufs[m_frame], VK_PIPELINE_BIND_POINT_COMPUTE, m_layout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
    vkCmdPushConstants(m_cmdBufs[m_frame], m_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, pushSize, pPushData);
    vkCmdDispatch(m_cmdBufs[m_frame], x, y, z);

    if (vkEndCommandBuffer(m_cmdBufs[m_frame]) != VK_SUCCESS){
        throw std::runtime_error("Failed to end commandBuffer");
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_cmdBufs[m_frame];
    if (vkQueueSubmit(m_vulDevice.computeQueue(), 1, &submitInfo, m_fences[m_frame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    m_frame = (m_frame + 1) % VulSwapChain::MAX_FRAMES_IN_FLIGHT;
}

}

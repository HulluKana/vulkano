#include "vul_buffer.hpp"
#include "vul_debug_tools.hpp"
#include "vul_pipeline.hpp"
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <vul_rt_pipeline.hpp>
#include <vulkan/vulkan_core.h>
#include <iostream>

using namespace vulB;
namespace vul {

VulRtPipeline::VulRtPipeline(const vulB::VulDevice &vulDevice, const std::string &raygenShader, const std::vector<std::string> &missShaders,
                const std::vector<std::string> &closestHitShaders, const std::vector<std::string> &anyHitShaders,
                const std::vector<std::string> &intersectionShaders, const std::vector<HitGroup> &hitGroups,
                const std::vector<VkDescriptorSetLayout> &setLayouts) : m_vulDevice{vulDevice}
{
    createPipeline(raygenShader, missShaders, closestHitShaders, anyHitShaders, intersectionShaders, hitGroups, setLayouts);
    createSBT(static_cast<uint32_t>(missShaders.size()), static_cast<uint32_t>(hitGroups.size()));
}

VulRtPipeline::~VulRtPipeline()
{
    vkDestroyPipeline(m_vulDevice.device(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_vulDevice.device(), m_layout, nullptr);
}

void VulRtPipeline::traceRays(uint32_t width, uint32_t height, uint32_t pushConstantSize, void *pushConstant, const std::vector<VkDescriptorSet> &descSets,
                VkCommandBuffer cmdBuf)
{
    VUL_PROFILE_FUNC()

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_pipeline);
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_layout, 0, static_cast<uint32_t>(descSets.size()), descSets.data(), 0, nullptr);
    if (pushConstantSize > 0) vkCmdPushConstants(cmdBuf, m_layout, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
            VK_SHADER_STAGE_MISS_BIT_KHR, 0, pushConstantSize, pushConstant);
    vkCmdTraceRaysKHR(cmdBuf, &m_rgenRegion, &m_rmissRegion, &m_rhitRegion, &m_callRegion, width, height, 1);
}

void VulRtPipeline::createPipeline(const std::string &raygenShader, const std::vector<std::string> &missShaders,
                const std::vector<std::string> &closestHitShaders, const std::vector<std::string> &anyHitShaders,
                const std::vector<std::string> &intersectionShaders, const std::vector<HitGroup> &hitGroups,
                const std::vector<VkDescriptorSetLayout> &setLayouts)
{
    const uint32_t missIdx = 1;
    const uint32_t closestIdx = missIdx + missShaders.size();
    const uint32_t anyHitIdx = closestIdx + closestHitShaders.size();
    const uint32_t intersectionIdx = anyHitIdx + anyHitShaders.size();
    const uint32_t shadersCount = intersectionIdx + intersectionShaders.size();

    std::vector<VkPipelineShaderStageCreateInfo> stages(shadersCount);
    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.pName = "main";

    VkRayTracingShaderGroupCreateInfoKHR group{};
    group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.closestHitShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;

    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    vulB::VulPipeline::createShaderModule(m_vulDevice, raygenShader, &stage.module);
    stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    stages[0] = stage;

    group.generalShader = 0;
    m_shaderGroups.push_back(group);

    for (size_t i = 0; i < missShaders.size(); i++) {
        vulB::VulPipeline::createShaderModule(m_vulDevice, missShaders[i], &stage.module);
        stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
        stages[i + missIdx] = stage;

        group.generalShader = i + missIdx;
        m_shaderGroups.push_back(group);
    }
    for (size_t i = 0; i < closestHitShaders.size(); i++) {
        vulB::VulPipeline::createShaderModule(m_vulDevice, closestHitShaders[i], &stage.module);
        stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        stages[i + closestIdx] = stage;
    }
    for (size_t i = 0; i < anyHitShaders.size(); i++) {
        vulB::VulPipeline::createShaderModule(m_vulDevice, anyHitShaders[i], &stage.module);
        stage.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
        stages[i + anyHitIdx] = stage;
    }
    for (size_t i = 0; i < intersectionShaders.size(); i++) {
        vulB::VulPipeline::createShaderModule(m_vulDevice, intersectionShaders[i], &stage.module);
        stage.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
        stages[i + intersectionIdx] = stage;
    }

    group.generalShader = VK_SHADER_UNUSED_KHR;
    for (const HitGroup &hit : hitGroups) {
        if (hit.intersectionIdx >= 0) group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
        else group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;

        if (hit.closestHitIdx >= 0) group.closestHitShader = hit.closestHitIdx + closestIdx;
        else group.closestHitShader = VK_SHADER_UNUSED_KHR;
        if (hit.anyHitIdx >= 0) group.anyHitShader = hit.anyHitIdx + anyHitIdx;
        else group.anyHitShader = VK_SHADER_UNUSED_KHR;
        if (hit.intersectionIdx >= 0) group.intersectionShader = hit.intersectionIdx + intersectionIdx;
        else group.intersectionShader = VK_SHADER_UNUSED_KHR;
        m_shaderGroups.push_back(group);
    }

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR
        | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
    pushRange.size = m_vulDevice.properties.limits.maxPushConstantsSize;
    pushRange.offset = 0;

    VkPipelineLayoutCreateInfo layoutCrI{};
    layoutCrI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCrI.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    layoutCrI.pSetLayouts = setLayouts.data();
    layoutCrI.pushConstantRangeCount = 1;
    layoutCrI.pPushConstantRanges = &pushRange;
    vkCreatePipelineLayout(m_vulDevice.device(), &layoutCrI, nullptr, &m_layout);

    VkRayTracingPipelineCreateInfoKHR piCrIn{};
    piCrIn.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    piCrIn.stageCount = shadersCount;
    piCrIn.pStages = stages.data();
    piCrIn.groupCount = static_cast<uint32_t>(m_shaderGroups.size());
    piCrIn.pGroups = m_shaderGroups.data();
    piCrIn.maxPipelineRayRecursionDepth = 2;
    piCrIn.layout = m_layout;
    vkCreateRayTracingPipelinesKHR(m_vulDevice.device(), {}, {}, 1, &piCrIn, nullptr, &m_pipeline);

    for (VkPipelineShaderStageCreateInfo &stage : stages) vkDestroyShaderModule(m_vulDevice.device(), stage.module, nullptr);

    VUL_NAME_VK(m_pipeline)    
    VUL_NAME_VK(m_layout)    
}

void VulRtPipeline::createSBT(uint32_t missCount, uint32_t hitCount)
{
    std::function<uint32_t(uint32_t, uint32_t)> alignUp = [](uint32_t victim, uint32_t murderer) {return (victim + murderer - 1) & ~(victim - 1);};

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties{};
    rtProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 prop2{};
    prop2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    prop2.pNext = &rtProperties;
    vkGetPhysicalDeviceProperties2(m_vulDevice.getPhysicalDevice(), &prop2);

    const uint32_t handleCount = 1 + missCount + hitCount; 
    const uint32_t handleSize = rtProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAligned = alignUp(handleSize, rtProperties.shaderGroupHandleAlignment);

    m_rgenRegion.stride = alignUp(handleSizeAligned, rtProperties.shaderGroupBaseAlignment);
    m_rgenRegion.size = m_rgenRegion.stride;
    m_rmissRegion.stride = handleSizeAligned;
    m_rmissRegion.size = alignUp(missCount * handleSizeAligned, rtProperties.shaderGroupBaseAlignment);
    m_rhitRegion.stride = handleSizeAligned;
    m_rhitRegion.size = alignUp(hitCount * handleSizeAligned, rtProperties.shaderGroupBaseAlignment);

    const size_t dataSize = handleSize * handleCount;
    std::vector<uint8_t> handles(dataSize);
    if (vkGetRayTracingShaderGroupHandlesKHR(m_vulDevice.device(), m_pipeline, 0, handleCount, dataSize, handles.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to get ray tracing shader group handles");

    VkDeviceSize sbtSize = m_rgenRegion.size + m_rmissRegion.size + m_rhitRegion.size + m_callRegion.size;
    m_SBTBuffer = std::make_unique<VulBuffer>(m_vulDevice);
    m_SBTBuffer->keepEmpty(1, sbtSize);
    m_SBTBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_sbt | VulBuffer::usage_getAddress | VulBuffer::usage_transferDst));

    VkDeviceAddress sbtAddress = m_SBTBuffer->getBufferAddress();
    m_rgenRegion.deviceAddress = sbtAddress;
    m_rmissRegion.deviceAddress = sbtAddress + m_rgenRegion.size;
    m_rhitRegion.deviceAddress = sbtAddress + m_rgenRegion.size + m_rmissRegion.size;

    std::function<uint8_t *(uint32_t)> getHandle = [&](uint32_t idx) {return handles.data() + idx * handleSize;};

    uint8_t *pSbtData = new uint8_t[sbtSize];
    uint8_t *pData = pSbtData;
    uint32_t handleIdx{};

    memcpy(pData, getHandle(handleIdx++), handleSize);
    pData = pSbtData + m_rgenRegion.size;
    for (uint32_t i = 0; i < missCount; i++) {
        memcpy(pData, getHandle(handleIdx++), handleSize);
        pData += m_rmissRegion.stride;
    }
    pData = pSbtData + m_rgenRegion.size + m_rmissRegion.size;
    for (uint32_t i = 0; i < hitCount; i++) {
        memcpy(pData, getHandle(handleIdx++), handleSize);
        pData += m_rhitRegion.stride;
    }
    
    m_SBTBuffer->writeData(pSbtData, sbtSize, 0);
    delete[] pSbtData;

    VUL_NAME_VK(m_SBTBuffer->getBuffer())
}

}

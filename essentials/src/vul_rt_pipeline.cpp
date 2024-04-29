#include "vul_debug_tools.hpp"
#include "vul_pipeline.hpp"
#include <vul_rt_pipeline.hpp>
#include <vulkan/vulkan_core.h>

using namespace vulB;
namespace vul {

VulRtPipeline::VulRtPipeline(vulB::VulDevice &vulDevice, const std::vector<std::string> &raygenShaders, const std::vector<std::string> &missShaders,
                const std::vector<std::string> &closestHitShaders, const std::vector<VkDescriptorSetLayout> &setLayouts) : m_vulDevice{vulDevice}
{
    createPipeline(raygenShaders, missShaders, closestHitShaders, setLayouts);
}

VulRtPipeline::~VulRtPipeline()
{
    vkDestroyPipeline(m_vulDevice.device(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_vulDevice.device(), m_layout, nullptr);
}

void VulRtPipeline::createPipeline(const std::vector<std::string> &raygenShaders, const std::vector<std::string> &missShaders,
        const std::vector<std::string> &closestHitShaders, const std::vector<VkDescriptorSetLayout> &setLayouts)
{
    const uint32_t raygenIdx = 0;
    const uint32_t missIdx = raygenIdx + raygenShaders.size();
    const uint32_t closestIdx = missIdx + missShaders.size();
    const uint32_t shadersCount = closestIdx + closestHitShaders.size();

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
    for (size_t i = 0; i < raygenShaders.size(); i++) {
        vulB::VulPipeline::createShaderModule(m_vulDevice, raygenShaders[i], &stage.module);
        stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        stages[i + raygenIdx] = stage;

        group.generalShader = i + raygenIdx;
        m_shaderGroups.push_back(group);
    }
    for (size_t i = 0; i < missShaders.size(); i++) {
        vulB::VulPipeline::createShaderModule(m_vulDevice, missShaders[i], &stage.module);
        stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
        stages[i + missIdx] = stage;

        group.generalShader = i + missIdx;
        m_shaderGroups.push_back(group);
    }
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    group.generalShader = VK_SHADER_UNUSED_KHR;
    for (size_t i = 0; i < closestHitShaders.size(); i++) {
        vulB::VulPipeline::createShaderModule(m_vulDevice, closestHitShaders[i], &stage.module);
        stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        stages[i + closestIdx] = stage;

        group.closestHitShader = i + closestIdx;
        m_shaderGroups.push_back(group);
    }

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;
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

}

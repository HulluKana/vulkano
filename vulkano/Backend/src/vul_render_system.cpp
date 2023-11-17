#include"../Headers/vul_render_system.hpp"
#include"../Headers/vul_settings.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>
#include<glm/gtc/constants.hpp>

#include<stdexcept>
#include<iostream>

using namespace vulB;
namespace vul{

RenderSystem::RenderSystem(VulDevice &device) : vulDevice{device}
{
}

void RenderSystem::init(std::vector<VkDescriptorSetLayout> setLayouts, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat)
{
    createPipelineLayout(setLayouts);
    createPipeline(colorAttachmentFormat, depthAttachmentFormat);
}

RenderSystem::~RenderSystem()
{
    vkDestroyPipelineLayout(vulDevice.device(), pipelineLayout, nullptr);
}

void RenderSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout> setLayouts)
{
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = vulDevice.properties.limits.maxPushConstantsSize; 

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(vulDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS){
        throw std::runtime_error("Failed to create pipelineLayout in RenderSystem.cpp file");
    }
}

void RenderSystem::createPipeline(VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat)
{
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    VulPipeline::defaultPipeLineConfigInfo(pipelineConfig); 

    pipelineConfig.renderPass = nullptr; 
    pipelineConfig.pipelineLayout = pipelineLayout;
    vulPipeline = std::make_unique<VulPipeline>(vulDevice, vertShaderName, fragShaderName, pipelineConfig, colorAttachmentFormat, depthAttachmentFormat);
}

void RenderSystem::render(std::vector<Vul3DObject> &objects, std::vector<VkDescriptorSet> &descriptorSets, VkCommandBuffer &commandBuffer)
{
    vulPipeline->bind(commandBuffer);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);

    for (Vul3DObject &obj : objects){
        if (obj.renderSystemIndex != index) continue;
        void *pushData;
        uint32_t pushDataSize;
        DefaultPushConstantInputData defaultData; // Has to be outside the if statement to prevent it from going out of scope too early and making the pushData pointer point to freed memory
        if (!obj.pCustomPushData){
            defaultData.modelMatrix = obj.transform.transformMat();
            defaultData.normalMatrix = obj.transform.normalMatrix();
            defaultData.color = obj.color;
            defaultData.isLight = obj.isLight;
            defaultData.lightIndex = obj.lightIndex;
            defaultData.specularExponent = obj.specularExponent;
            defaultData.texIndex = (obj.hasTexture) ? obj.textureIndex : -1;
            pushData = reinterpret_cast<void *>(&defaultData);
            pushDataSize = sizeof(defaultData);
        } else{
            pushData = obj.pCustomPushData;
            pushDataSize = obj.customPushDataSize;
        }

        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, pushDataSize, pushData);

        obj.model->bind(commandBuffer);
        obj.model->draw(commandBuffer);
    }
}

void RenderSystem::render(std::vector<VulScreenObject> &objects, std::vector<VkDescriptorSet> &descriptorSets, VkCommandBuffer &commandBuffer)
{
    vulPipeline->bind(commandBuffer);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);

    for (VulScreenObject &obj : objects){
        if (obj.renderSystemIndex != index) continue;
        void *pushData;
        uint32_t pushDataSize;
        DefaultPushConstantInputData defaultData; // Has to be outside the if statement to prevent it from going out of scope too early and making the pushData pointer point to freed memory
        if (!obj.pCustomPushData){
            defaultData.color = obj.color;
            defaultData.texIndex = (obj.hasTexture) ? obj.textureIndex : -1;
            pushData = reinterpret_cast<void *>(&defaultData);
            pushDataSize = sizeof(defaultData);
        } else{
            pushData = obj.pCustomPushData;
            pushDataSize = obj.customPushDataSize;
        }

        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, pushDataSize, pushData);

        obj.model->bind(commandBuffer);
        obj.model->draw(commandBuffer);
    }
}

}
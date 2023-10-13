#include"../Headers/vul_simple_render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>
#include<glm/gtc/constants.hpp>

#include<stdexcept>
#include<iostream>

namespace vulB{

struct simplePushConstantData {
    glm::mat4 modelMatrix{1.0f};
    glm::mat4 normalMatrix{1.0f};
    glm::vec3 color{1.0f}; 
    int isLight;
    int lightIndex;
    float specularExponent = 0.0f;
    int texIndex;
};

SimpleRenderSystem::SimpleRenderSystem(VulDevice &device) : vulDevice{device}
{
}

void SimpleRenderSystem::init(VkRenderPass renderpass, std::vector<VkDescriptorSetLayout> setLayouts, const std::string &shadersFolder)
{
    createPipelineLayout(setLayouts);
    createPipeline(renderpass, shadersFolder);
}

SimpleRenderSystem::~SimpleRenderSystem()
{
    vkDestroyPipelineLayout(vulDevice.device(), pipelineLayout, nullptr);
}

void SimpleRenderSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout> setLayouts)
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
        throw std::runtime_error("Failed to create pipelineLayout in SimpleRenderSystem.cpp file");
    }
}

void SimpleRenderSystem::createPipeline(VkRenderPass renderPass, const std::string &shadersFolder)
{
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    std::string vertShader("/simple.vert.spv");
    std::string fragShader("/simple.frag.spv");
    vertShader = shadersFolder + vertShader;
    fragShader = shadersFolder + fragShader;

    PipelineConfigInfo pipelineConfig{};
    VulPipeline::defaultPipeLineConfigInfo(pipelineConfig); 

    pipelineConfig.renderPass = renderPass; 
    pipelineConfig.pipelineLayout = pipelineLayout;
    vulPipeline = std::make_unique<VulPipeline>(vulDevice, vertShader, fragShader, pipelineConfig);
}

void SimpleRenderSystem::renderObjects(std::vector<VulObject> &objects, std::vector<VkDescriptorSet> &descriptorSets, VkCommandBuffer &commandBuffer, int maxLights)
{
    vulPipeline->bind(commandBuffer);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);

    int lightIndex = 0;
    for (VulObject &obj : objects){
        simplePushConstantData push{};
        push.normalMatrix = obj.transform.normalMatrix();
        push.modelMatrix = obj.transform.transformMat();
        push.color = obj.color;
        push.isLight = (obj.isLight) ? 1 : 0;
        push.lightIndex = lightIndex;
        if (obj.isLight && lightIndex < maxLights - 1) lightIndex++;
        push.specularExponent = obj.specularExponent;
        push.texIndex = (obj.hasTexture) ? (int)obj.textureIndex : -1;

        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,  sizeof(simplePushConstantData), &push);

        obj.model->bind(commandBuffer);
        obj.model->draw(commandBuffer);
    }
}

}
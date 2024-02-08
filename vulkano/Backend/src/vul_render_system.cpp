#include"../Headers/vul_render_system.hpp"
#include"../Headers/vul_settings.hpp"
#include <vulkan/vulkan_core.h>

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

void RenderSystem::init(std::vector<VkDescriptorSetLayout> setLayouts, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat, bool is2D)
{
    createPipelineLayout(setLayouts);
    createPipeline(colorAttachmentFormat, depthAttachmentFormat, is2D);
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

void RenderSystem::createPipeline(VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat, bool is2D)
{
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    VulPipeline::defaultPipeLineConfigInfo(pipelineConfig); 

    pipelineConfig.renderPass = nullptr; 
    pipelineConfig.pipelineLayout = pipelineLayout;
    if (!is2D){
        pipelineConfig.attributeDescriptions = {{0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0}, {1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0}, {2, 2, VK_FORMAT_R32G32_SFLOAT}};
        pipelineConfig.bindingDescriptions = {{0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX}, {1, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX}, {2, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}};
    } else{
        pipelineConfig.attributeDescriptions = {{0, 0, VK_FORMAT_R32G32_SFLOAT, 0}, {1, 1, VK_FORMAT_R32G32_SFLOAT, 0}};
        pipelineConfig.bindingDescriptions = {{0, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}, {1, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}};
    }

    vulPipeline = std::make_unique<VulPipeline>(vulDevice, vertShaderName, fragShaderName, pipelineConfig, colorAttachmentFormat, depthAttachmentFormat);
}

void RenderSystem::render(const Scene &scene, std::vector<VkDescriptorSet> &descriptorSets, VkCommandBuffer &commandBuffer)
{
    vulPipeline->bind(commandBuffer);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);

    std::vector<VkBuffer> vertexBuffers = {scene.vertexBuffer->getBuffer(), scene.normalBuffer->getBuffer(), scene.uvBuffer->getBuffer()};
    std::vector<VkDeviceSize> offsets = {0, 0, 0};
    vkCmdBindVertexBuffers(commandBuffer, 0, static_cast<uint32_t>(vertexBuffers.size()), vertexBuffers.data(), offsets.data());
    vkCmdBindIndexBuffer(commandBuffer, scene.indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

    int idx = 0;
    for (const GltfLoader::GltfNode &node : scene.nodes){
        const GltfLoader::GltfPrimMesh &mesh = scene.meshes[node.primMesh];

        void *pushData;
        uint32_t pushDataSize;
        DefaultPushConstantInputData defaultData; // Has to be outside the if statement to prevent it from going out of scope too early and making the pushData pointer point to freed memory
        defaultData.modelMatrix = node.worldMatrix;
        defaultData.normalMatrix = glm::mat4(1.0f);
        defaultData.matIdx = mesh.materialIndex;
        pushData = reinterpret_cast<void *>(&defaultData);
        pushDataSize = sizeof(defaultData);

        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, pushDataSize, pushData);
        vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, mesh.firstIndex, mesh.vertexOffset, 0);

        idx++;
    }
}

void RenderSystem::render(Object2D &obj, std::vector<VkDescriptorSet> &descriptorSets, VkCommandBuffer &commandBuffer)
{
    vulPipeline->bind(commandBuffer);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);

    if (obj.customPushDataSize > 0) vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, obj.customPushDataSize, obj.pCustomPushData);

    obj.bind(commandBuffer);
    obj.draw(commandBuffer);
}

void RenderSystem::render(std::vector<Object2D> &objs, std::vector<VkDescriptorSet> &descriptorSets, VkCommandBuffer &commandBuffer)
{
    vulPipeline->bind(commandBuffer);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);

    objs[0].bind(commandBuffer);
    for (size_t i = 0; i < objs.size(); i++){
        if (objs[i].customPushDataSize > 0) vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
            0, objs[i].customPushDataSize, objs[i].pCustomPushData);
        objs[i].draw(commandBuffer);
    }
}

}

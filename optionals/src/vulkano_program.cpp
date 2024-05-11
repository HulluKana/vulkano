#include "vul_2d_object.hpp"
#include "vul_acceleration_structure.hpp"
#include "vul_descriptors.hpp"
#include <vul_debug_tools.hpp>
#include "vul_renderer.hpp"
#include <cstdlib>
#include <vulkan/vulkan_core.h>
#include<vulkano_program.hpp>

#include<imgui.h>
#include <memory>
#include <stdexcept>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>
#include<glm/gtc/constants.hpp>

#include <iostream>

using namespace vulB;
namespace vul{

Vulkano::Vulkano(uint32_t width, uint32_t height, std::string name) : m_vulWindow{(int)width, (int)height, name}
{
    m_globalPool = VulDescriptorPool::Builder(m_vulDevice)
        .setMaxSets(m_vulDevice.properties.limits.maxBoundDescriptorSets)
        .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
        .build();

    m_vulGUI.initImGui(m_vulWindow.getGLFWwindow(), m_globalPool->getDescriptorPoolReference(), vulRenderer, m_vulDevice);

    m_currentTime = glfwGetTime();
    m_prevWindowSize = vulRenderer.getSwapChainExtent();
}

Vulkano::~Vulkano()
{
    letVulkanoFinish();
    m_vulGUI.destroyImGui();
}

VkCommandBuffer Vulkano::startFrame()
{
    VUL_PROFILE_FUNC()
    glfwPollEvents();
        
    double newTime = glfwGetTime();
    double idleStartTime = glfwGetTime();
    while (1){
        if (1.0 / (newTime - m_currentTime) < settings::maxFps) break;
        newTime = glfwGetTime();
    }
    m_idleTime = glfwGetTime() - idleStartTime;
    m_frameTime = newTime - m_currentTime;
    m_currentTime = newTime;

    ImGuiIO &io = ImGui::GetIO(); (void)io;
    if (!io.WantCaptureKeyboard) cameraController.modifyValues(m_vulWindow.getGLFWwindow(), cameraTransform);
    if (!io.WantCaptureMouse && !io.WantCaptureKeyboard) cameraController.rotate(m_vulWindow.getGLFWwindow(), m_frameTime, cameraTransform, vulRenderer.getSwapChainExtent().width, vulRenderer.getSwapChainExtent().height);
    if (!io.WantCaptureKeyboard) cameraController.move(m_vulWindow.getGLFWwindow(), m_frameTime, cameraTransform);

    float aspect = vulRenderer.getAspectRatio();
    if (settings::cameraProperties.hasPerspective) camera.setPerspectiveProjection(settings::cameraProperties.fovY, aspect, settings::cameraProperties.nearPlane, settings::cameraProperties.farPlane);
    else camera.setOrthographicProjection(settings::cameraProperties.leftPlane, settings::cameraProperties.rightPlane, settings::cameraProperties.topPlane,
                                            settings::cameraProperties.bottomPlane, settings::cameraProperties.nearPlane, settings::cameraProperties.farPlane);

    camera.setViewXYZ(cameraTransform.pos, cameraTransform.rot);

    if (VkCommandBuffer commandBuffer = vulRenderer.beginFrame()){
        m_prevWindowSize = vulRenderer.getSwapChainExtent();

        if (!cameraController.hideGUI) m_vulGUI.startFrame();

        return commandBuffer;
    }

    return nullptr;
}

bool Vulkano::endFrame(VkCommandBuffer commandBuffer)
{
    VUL_PROFILE_FUNC()
    VulRenderer::SwapChainImageMode prevSwapChainMode{};
    VulRenderer::DepthImageMode prevDepthImageMode{};
    size_t prevAttachmentImageCount = 0;
    bool firstPass = true;
    bool prevSampleFromDepth = false;
    if (hasScene){
        for (const RenderData &renderData : renderDatas){
            if (!renderData.is3d) continue;

            if (firstPass || renderData.swapChainImageMode != prevSwapChainMode || renderData.depthImageMode != prevDepthImageMode || renderData.attachmentImages[vulRenderer.getFrameIndex()].size() > 0) {
                if (!firstPass) vulRenderer.stopRendering(commandBuffer);
                if (prevSampleFromDepth) for (const auto &depthImage : vulRenderer.getDepthImages()) depthImage->transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, commandBuffer);
                if (renderData.sampleFromDepth) for (const auto &depthImage : vulRenderer.getDepthImages()) depthImage->transitionImageLayout(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandBuffer);
                vulRenderer.beginRendering(commandBuffer, renderData.attachmentImages[vulRenderer.getFrameIndex()], renderData.swapChainImageMode, renderData.depthImageMode, settings::renderWidth, settings::renderHeight);
                prevSwapChainMode = renderData.swapChainImageMode;
                prevDepthImageMode = renderData.depthImageMode;
                prevAttachmentImageCount = renderData.attachmentImages[vulRenderer.getFrameIndex()].size();
                firstPass = false;
                prevSampleFromDepth = renderData.sampleFromDepth;
            }
            std::vector<VkDescriptorSet> descriptorSets;
            for (const std::shared_ptr<VulDescriptorSet> &descriptorSet : renderData.descriptorSets[vulRenderer.getFrameIndex()]) descriptorSets.push_back(descriptorSet->getSet());
            std::vector<VkBuffer> vertexBuffers = {scene.vertexBuffer->getBuffer(), scene.normalBuffer->getBuffer(), scene.tangentBuffer->getBuffer(),
                scene.uvBuffer->getBuffer()};
            renderData.pipeline->draw(commandBuffer, descriptorSets, vertexBuffers, scene.indexBuffer->getBuffer(), renderData.drawDatas);
        }
    }
    if (object2Ds.size() > 0) {
        for (const RenderData &renderData : renderDatas){
            if (renderData.is3d) continue;

            if (firstPass || renderData.swapChainImageMode != prevSwapChainMode || renderData.depthImageMode != prevDepthImageMode || renderData.attachmentImages[vulRenderer.getFrameIndex()].size() > 0) {
                if (!firstPass) vulRenderer.stopRendering(commandBuffer);
                if (prevSampleFromDepth) for (const auto &depthImage : vulRenderer.getDepthImages()) depthImage->transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, commandBuffer);
                vulRenderer.beginRendering(commandBuffer, renderData.attachmentImages[vulRenderer.getFrameIndex()], renderData.swapChainImageMode, renderData.depthImageMode, settings::renderWidth, settings::renderHeight);
                prevSwapChainMode = renderData.swapChainImageMode;
                prevDepthImageMode = renderData.depthImageMode;
                prevAttachmentImageCount = renderData.attachmentImages[vulRenderer.getFrameIndex()].size();
                firstPass = false;
            }

            std::vector<VkDescriptorSet> descriptorSets;
            for (const std::shared_ptr<VulDescriptorSet> &descriptorSet : renderData.descriptorSets[vulRenderer.getFrameIndex()]) descriptorSets.push_back(descriptorSet->getSet());
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderData.pipeline->getPipeline());
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderData.pipeline->getPipelineLayout(), 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
            for (Object2D &obj2d : object2Ds) {
                obj2d.bind(commandBuffer);
                obj2d.draw(commandBuffer);
            }
        }
    }

    if (firstPass || VulRenderer::SwapChainImageMode::preservePreviousStoreCurrent != prevSwapChainMode || VulRenderer::DepthImageMode::noDepthImage != prevDepthImageMode || prevAttachmentImageCount > 0) {
        if (!firstPass) vulRenderer.stopRendering(commandBuffer);
        vulRenderer.beginRendering(commandBuffer, {}, VulRenderer::SwapChainImageMode::preservePreviousStoreCurrent, VulRenderer::DepthImageMode::noDepthImage, settings::renderWidth, settings::renderHeight);
    }
    if (!cameraController.hideGUI) m_vulGUI.endFrame(commandBuffer);
    vulRenderer.stopRendering(commandBuffer);
    vulRenderer.endFrame();

    return m_vulWindow.shouldClose();
}

void Vulkano::createSquare(float x, float y, float width, float height)
{
    Object2D object;
    object.addSquare(m_vulDevice, x, y, width, height);
    object2Ds.push_back(std::move(object));
}

void Vulkano::createTriangle(glm::vec2 corner1, glm::vec2 corner2, glm::vec2 corner3)
{
    Object2D object;
    object.addTriangle(m_vulDevice, corner1, corner2, corner3);
    object2Ds.push_back(std::move(object));
}

std::unique_ptr<VulDescriptorSet> Vulkano::createDescriptorSet(const std::vector<Descriptor> &descriptors) const
{
    VulDescriptorSetLayout::Builder layoutBuilder = VulDescriptorSetLayout::Builder(m_vulDevice);
    for (size_t i = 0; i < descriptors.size(); i++){
        int stageFlags = 0;
        for (size_t j = 0; j < descriptors[i].stages.size(); j++)
            stageFlags |= static_cast<int>(descriptors[i].stages[j]);
        VkDescriptorType type{};
        if (descriptors[i].type == DescriptorType::ubo) type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        else if (descriptors[i].type == DescriptorType::ssbo) type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        else if (descriptors[i].type == DescriptorType::combinedImgSampler ||
            descriptors[i].type == DescriptorType::spCombinedImgSampler || descriptors[i].type == DescriptorType::upCombinedImgSampler)
            type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        else if (descriptors[i].type == DescriptorType::storageImage) type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        else if (descriptors[i].type == DescriptorType::accelerationStructure) type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        layoutBuilder.addBinding(i, type, stageFlags, descriptors[i].count);
    }
    std::shared_ptr<VulDescriptorSetLayout> layout = layoutBuilder.build();

    std::unique_ptr<VulDescriptorSet> set = std::make_unique<VulDescriptorSet>(layout, *m_globalPool);
    std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfosStorage;
    std::vector<std::vector<VkDescriptorImageInfo>> imageInfosStorage;
    std::vector<std::vector<VkWriteDescriptorSetAccelerationStructureKHR>> tlasInfosStorage;
    for (size_t i = 0; i < descriptors.size(); i++){
        const Descriptor &desc = descriptors[i];
        if (desc.type == DescriptorType::ubo || desc.type == DescriptorType::ssbo){
            const VulBuffer *buffer = static_cast<const VulBuffer *>(desc.content);
            std::vector<VkDescriptorBufferInfo> bufferInfos(desc.count);
            for (uint32_t j = 0; j < desc.count; j++)
                bufferInfos[j] = buffer[j].getDescriptorInfo();
            bufferInfosStorage.push_back(bufferInfos);
            set->writeBuffer(i, bufferInfosStorage[bufferInfosStorage.size() - 1].data(), desc.count);
        }
        if (desc.type == DescriptorType::combinedImgSampler){
            const VulImage *image = static_cast<const VulImage *>(desc.content);
            std::vector<VkDescriptorImageInfo> imageInfos(desc.count);
            for (uint32_t j = 0; j < desc.count; j++){
                imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfos[j].imageView = image[j].getImageView();
                imageInfos[j].sampler = image[j].vulSampler->getSampler();
            }
            imageInfosStorage.push_back(imageInfos);
            set->writeImage(i, imageInfosStorage[imageInfosStorage.size() - 1].data(), desc.count);
        }
        if (desc.type == DescriptorType::spCombinedImgSampler){
            const std::shared_ptr<VulImage> *image = static_cast<const std::shared_ptr<VulImage> *>(desc.content);
            std::vector<VkDescriptorImageInfo> imageInfos(desc.count);
            for (uint32_t j = 0; j < desc.count; j++){
                imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfos[j].imageView = image[j]->getImageView();
                imageInfos[j].sampler = image[j]->vulSampler->getSampler();
            }
            imageInfosStorage.push_back(imageInfos);
            set->writeImage(i, imageInfosStorage[imageInfosStorage.size() - 1].data(), desc.count);
        }
        if (desc.type == DescriptorType::upCombinedImgSampler){
            const std::unique_ptr<VulImage> *image = static_cast<const std::unique_ptr<VulImage> *>(desc.content);
            std::vector<VkDescriptorImageInfo> imageInfos(desc.count);
            for (uint32_t j = 0; j < desc.count; j++){
                imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfos[j].imageView = image[j]->getImageView();
                imageInfos[j].sampler = image[j]->vulSampler->getSampler();
            }
            imageInfosStorage.push_back(imageInfos);
            set->writeImage(i, imageInfosStorage[imageInfosStorage.size() - 1].data(), desc.count);
        }
        if (desc.type == DescriptorType::storageImage){
            const VulImage *image = static_cast<const VulImage *>(desc.content);
            std::vector<VkDescriptorImageInfo> imageInfos(desc.count);
            for (uint32_t j = 0; j < desc.count; j++){
                imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageInfos[j].imageView = image[j].getImageView();
            }
            imageInfosStorage.push_back(imageInfos);
            set->writeImage(i, imageInfosStorage[imageInfosStorage.size() - 1].data(), desc.count);
        }
        if (desc.type == DescriptorType::accelerationStructure) {
            const VulAs *as = static_cast<const VulAs *>(desc.content);
            std::vector<VkWriteDescriptorSetAccelerationStructureKHR> asInfos(desc.count);
            for (uint32_t j = 0; j < desc.count; j++) {
                asInfos[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
                asInfos[j].accelerationStructureCount = 1;
                asInfos[j].pAccelerationStructures = as->getPTlas();
            }
            tlasInfosStorage.push_back(asInfos);
            set->writeTlas(i, tlasInfosStorage[tlasInfosStorage.size() - 1].data(), desc.count);
        }
    }
    set->build();

    return set;
}
        
VulCompPipeline Vulkano::createNewComputePipeline(const std::vector<VkDescriptorSetLayout> &setLayouts, const std::string &compShaderName, uint32_t maxSubmitsInFlight)
{
    VulCompPipeline compPipeline(compShaderName, setLayouts, m_vulDevice, maxSubmitsInFlight);
    return compPipeline;
}

}

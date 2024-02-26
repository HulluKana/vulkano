#include"../../vulkano_program.hpp"

#include"../../../3rdParty/imgui/imgui.h"
#include <memory>
#include <stdexcept>
#include <variant>
#include <vulkan/vulkan_core.h>

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
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 50)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 50)
        .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 50)
        .build();

    uint8_t *data = new uint8_t[16]{255, 0, 255, 255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 0, 255, 255};
    std::shared_ptr<VulImage> emptyImage = std::make_shared<VulImage>(m_vulDevice);
    emptyImage->loadData(data, 2, 2, 4);
    emptyImage->createImage(true, true, false);
    for (uint32_t i = 0; i < MAX_TEXTURES; i++)
        images[i] = emptyImage;
}

Vulkano::~Vulkano()
{
    letVulkanoFinish();
    m_vulGUI.destroyImGui();
}

void Vulkano::initVulkano()
{
    for (uint32_t i = 0; i < imageCount; i++){
        if (!images[i]->usableByImGui) continue;

        Descriptor image{};
        image.type = DescriptorType::combinedTexSampler;
        image.content = images[i].get();
        image.stages = {ShaderStage::frag};

        descSetReturnVal retVal = createDescriptorSet({image});
        if (!retVal.succeeded) throw std::runtime_error("Failed to create imgui image descriptor sets");
        images[i]->setDescriptorSet(retVal.set.getSet());
        m_imGuiDescriptorSets.push_back(std::move(retVal.set));
    }

    m_vulGUI.initImGui(m_vulWindow.getGLFWwindow(), m_globalPool->getDescriptorPoolReference(), m_vulRenderer, m_vulDevice);

    m_currentTime = glfwGetTime();
    m_prevWindowSize = m_vulRenderer.getSwapChainExtent();
}

VkCommandBuffer Vulkano::startFrame()
{
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
    double renderPreparationStartTime = glfwGetTime();

    ImGuiIO &io = ImGui::GetIO(); (void)io;
    if (!io.WantCaptureKeyboard) cameraController.modifyValues(m_vulWindow.getGLFWwindow(), cameraTransform);
    if (!io.WantCaptureMouse && !io.WantCaptureKeyboard) cameraController.rotate(m_vulWindow.getGLFWwindow(), m_frameTime, cameraTransform, m_vulRenderer.getSwapChainExtent().width, m_vulRenderer.getSwapChainExtent().height);
    if (!io.WantCaptureKeyboard) cameraController.move(m_vulWindow.getGLFWwindow(), m_frameTime, cameraTransform);

    float aspect = m_vulRenderer.getAspectRatio();
    if (settings::cameraProperties.hasPerspective) camera.setPerspectiveProjection(settings::cameraProperties.fovY, aspect, settings::cameraProperties.nearPlane, settings::cameraProperties.farPlane);
    else camera.setOrthographicProjection(settings::cameraProperties.leftPlane, settings::cameraProperties.rightPlane, settings::cameraProperties.topPlane,
                                            settings::cameraProperties.bottomPlane, settings::cameraProperties.nearPlane, settings::cameraProperties.farPlane);

    camera.setViewYXZ(cameraTransform.pos, cameraTransform.rot);

    if (VkCommandBuffer commandBuffer = m_vulRenderer.beginFrame()){
        m_prevWindowSize = m_vulRenderer.getSwapChainExtent();

        m_vulRenderer.beginRendering(commandBuffer, settings::renderWidth, settings::renderHeight);
        if (!cameraController.hideGUI) m_vulGUI.startFrame();

        m_renderPreparationTime = glfwGetTime() - renderPreparationStartTime;
        return commandBuffer;
    }

    return NULL;
}

bool Vulkano::endFrame(VkCommandBuffer commandBuffer)
{
    int frameIdx = m_vulRenderer.getFrameIndex();

    double objRenderStartTime = glfwGetTime();
    if (hasScene){
        std::vector<VkDescriptorSet> defaultDescriptorSets;
        defaultDescriptorSets.push_back(mainDescriptorSets[frameIdx].getSet());
        renderSystem3D->render(scene, defaultDescriptorSets, commandBuffer);
    }
    if (object2Ds.size() > 0){
        std::vector<VkDescriptorSet> defaultDescriptorSets;
        defaultDescriptorSets.push_back(mainDescriptorSets[frameIdx].getSet());
        if (vul::settings::batchRender2Ds) renderSystem2D->render(object2Ds, defaultDescriptorSets, commandBuffer);
        else{
            for (Object2D &obj : object2Ds){
                renderSystem2D->render(obj, defaultDescriptorSets, commandBuffer);
            }
        }
    }
    m_objRenderTime = glfwGetTime() - objRenderStartTime;

    double guiRenderStartTime = glfwGetTime();
    if (!cameraController.hideGUI) m_vulGUI.endFrame(commandBuffer);
    m_GuiRenderTime = glfwGetTime() - guiRenderStartTime;

    double renderFinishStartTime = glfwGetTime();
    m_vulRenderer.stopRendering(commandBuffer);
    m_vulRenderer.endFrame();
    m_renderFinishingTime = glfwGetTime() - renderFinishStartTime;

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

Vulkano::descSetReturnVal Vulkano::createDescriptorSet(const std::vector<Descriptor> &descriptors)
{
    VulDescriptorSetLayout::Builder layoutBuilder = VulDescriptorSetLayout::Builder(m_vulDevice);
    for (size_t i = 0; i < descriptors.size(); i++){
        int stageFlags = 0;
        for (size_t j = 0; j < descriptors[i].stages.size(); j++)
            stageFlags |= static_cast<int>(descriptors[i].stages[j]);
        VkDescriptorType type;
        if (descriptors[i].type == DescriptorType::ubo) type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        else if (descriptors[i].type == DescriptorType::ssbo) type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        else if (descriptors[i].type == DescriptorType::combinedTexSampler ||
            descriptors[i].type == DescriptorType::spCombinedTexSampler)
            type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        else if (descriptors[i].type == DescriptorType::storageImage) type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        layoutBuilder.addBinding(i, type, stageFlags, descriptors[i].count);
    }
    std::unique_ptr<VulDescriptorSetLayout> layout = layoutBuilder.build();

    VulDescriptorSet set(*layout, *m_globalPool);
    std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfosStorage;
    std::vector<std::vector<VkDescriptorImageInfo>> imageInfosStorage;
    for (size_t i = 0; i < descriptors.size(); i++){
        const Descriptor &desc = descriptors[i];
        if (desc.type == DescriptorType::ubo || desc.type == DescriptorType::ssbo){
            VulBuffer *buffer = static_cast<VulBuffer *>(desc.content);
            std::vector<VkDescriptorBufferInfo> bufferInfos(desc.count);
            for (uint32_t j = 0; j < desc.count; j++)
                bufferInfos[j] = buffer[j].descriptorInfo();
            bufferInfosStorage.push_back(bufferInfos);
            set.writeBuffer(i, bufferInfosStorage[bufferInfosStorage.size() - 1].data(), desc.count);
        }
        if (desc.type == DescriptorType::combinedTexSampler){
            VulImage *image = static_cast<VulImage *>(desc.content);
            std::vector<VkDescriptorImageInfo> imageInfos(desc.count);
            for (uint32_t j = 0; j < desc.count; j++){
                imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfos[j].imageView = image[j].getImageView();
                imageInfos[j].sampler = image[j].getTextureSampler();
            }
            imageInfosStorage.push_back(imageInfos);
            set.writeImage(i, imageInfosStorage[imageInfosStorage.size() - 1].data(), desc.count);
        }
        if (desc.type == DescriptorType::spCombinedTexSampler){
            std::shared_ptr<VulImage> *image = static_cast<std::shared_ptr<VulImage> *>(desc.content);
            std::vector<VkDescriptorImageInfo> imageInfos(desc.count);
            for (uint32_t j = 0; j < desc.count; j++){
                imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfos[j].imageView = image[j]->getImageView();
                imageInfos[j].sampler = image[j]->getTextureSampler();
            }
            imageInfosStorage.push_back(imageInfos);
            set.writeImage(i, imageInfosStorage[imageInfosStorage.size() - 1].data(), desc.count);
        }
        if (desc.type == DescriptorType::storageImage){
            VulImage *image = static_cast<VulImage *>(desc.content);
            std::vector<VkDescriptorImageInfo> imageInfos(desc.count);
            for (uint32_t j = 0; j < desc.count; j++){
                imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageInfos[j].imageView = image[j].getImageView();
            }
            imageInfosStorage.push_back(imageInfos);
            set.writeImage(i, imageInfosStorage[imageInfosStorage.size() - 1].data(), desc.count);
        }
    }
    set.build();

    return {std::move(set), std::move(layout), set.hasSet()};
}

std::unique_ptr<RenderSystem> Vulkano::createNewRenderSystem(const std::vector<VkDescriptorSetLayout> &setLayouts, std::string vertShaderName, std::string fragShaderName, bool is2D)
{
    std::unique_ptr<RenderSystem> renderSystem = std::make_unique<RenderSystem>(m_vulDevice);
    renderSystem->init(setLayouts, vertShaderName, fragShaderName, m_vulRenderer.getSwapChainColorFormat(), m_vulRenderer.getSwapChainDepthFormat(), is2D);
    return renderSystem;
}
        
VulCompPipeline Vulkano::createNewComputePipeline(const std::vector<VkDescriptorSetLayout> &setLayouts, const std::string &compShaderName, uint32_t maxSubmitsInFlight)
{
    VulCompPipeline compPipeline(compShaderName, setLayouts, m_vulDevice, maxSubmitsInFlight);
    return compPipeline;
}

}

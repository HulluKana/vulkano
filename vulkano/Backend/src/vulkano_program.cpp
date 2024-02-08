#include"../../vulkano_program.hpp"

#include"../../../3rdParty/imgui/imgui.h"
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

}

Vulkano::~Vulkano()
{
    m_vulGUI.destroyImGui();
}

void Vulkano::initVulkano()
{
    m_globalPool = VulDescriptorPool::Builder(m_vulDevice)
        .setMaxSets(m_vulDevice.properties.limits.maxBoundDescriptorSets)
        .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VulSwapChain::MAX_FRAMES_IN_FLIGHT * 5)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VulSwapChain::MAX_FRAMES_IN_FLIGHT * MAX_TEXTURES * 2)
        .addPoolSize(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VulSwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VulSwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VulSwapChain::MAX_FRAMES_IN_FLIGHT * 12)
        .build();

    for (size_t i = 0; i < VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
        std::unique_ptr<VulBuffer> globalBuffer;
        globalBuffer = std::make_unique<VulBuffer>  (m_vulDevice, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_vulDevice.properties.limits.minUniformBufferOffsetAlignment);
        globalBuffer->map();
        m_uboBuffers.push_back(std::move(globalBuffer));
    }

    uint8_t *data = new uint8_t[16]{255, 0, 255, 255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 0, 255, 255};
    m_emptyImage.createTextureFromData(data, 2, 2);

    m_globalSetLayout = VulDescriptorSetLayout::Builder(m_vulDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, MAX_TEXTURES)
        .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
        .build();
    createGlobalDescriptorSets();

    m_imGuiSetLayout = VulDescriptorSetLayout::Builder(m_vulDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();
    createImGuiDescriptorSets();

    std::vector<VkDescriptorSetLayout> defaultSetLayouts{m_globalSetLayout->getDescriptorSetLayout()};
    createNewRenderSystem(defaultSetLayouts); // Default 3D render system

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
    if (!io.WantCaptureKeyboard) m_cameraController.modifyValues(m_vulWindow.getGLFWwindow(), m_cameraTransform);
    if (!io.WantCaptureMouse && !io.WantCaptureKeyboard) m_cameraController.rotate(m_vulWindow.getGLFWwindow(), m_frameTime, m_cameraTransform, m_vulRenderer.getSwapChainExtent().width, m_vulRenderer.getSwapChainExtent().height);
    if (!io.WantCaptureKeyboard) m_cameraController.move(m_vulWindow.getGLFWwindow(), m_frameTime, m_cameraTransform);

    float aspect = m_vulRenderer.getAspectRatio();
    if (settings::cameraProperties.hasPerspective) m_camera.setPerspectiveProjection(settings::cameraProperties.fovY, aspect, settings::cameraProperties.nearPlane, settings::cameraProperties.farPlane);
    else m_camera.setOrthographicProjection(settings::cameraProperties.leftPlane, settings::cameraProperties.rightPlane, settings::cameraProperties.topPlane,
                                            settings::cameraProperties.bottomPlane, settings::cameraProperties.nearPlane, settings::cameraProperties.farPlane);

    m_camera.setViewYXZ(m_cameraTransform.pos, m_cameraTransform.rot);

    if (VkCommandBuffer commandBuffer = m_vulRenderer.beginFrame()){
        m_prevWindowSize = m_vulRenderer.getSwapChainExtent();
        int frameIndex = m_vulRenderer.getFrameIndex();

        GlobalUbo ubo{};
        ubo.projectionMatrix = m_camera.getProjection();
        ubo.viewMatrix = m_camera.getView();
        ubo.cameraPosition = glm::vec4(m_cameraTransform.pos, 0.0f);

        ubo.ambientLightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.02f);
        ubo.numLights = scene.lightCount;
        for (int i = 0; i < scene.lightCount; i++){
            ubo.lightPositions[i] = scene.lightPositions[i];
            ubo.lightColors[i] = scene.lightColors[i];
        }

        m_uboBuffers[frameIndex]->writeToBuffer(&ubo, sizeof(ubo));

        m_vulRenderer.beginRendering(commandBuffer, settings::renderWidth, settings::renderHeight);
        if (!m_cameraController.hideGUI) m_vulGUI.startFrame();

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
        defaultDescriptorSets.push_back(m_globalDescriptorSets[frameIdx].getSet());
        renderSystems[0]->render(scene, defaultDescriptorSets, commandBuffer);
    }
    if (object2Ds.size() > 0){
        std::vector<VkDescriptorSet> defaultDescriptorSets;
        defaultDescriptorSets.push_back(m_globalDescriptorSets[frameIdx].getSet());
        if (vul::settings::batchRender2Ds) renderSystems[object2Ds[0].renderSystemIndex]->render(object2Ds, defaultDescriptorSets, commandBuffer);
        else{
            for (Object2D &obj : object2Ds){
                renderSystems[obj.renderSystemIndex]->render(obj, defaultDescriptorSets, commandBuffer);
            }
        }
    }
    m_objRenderTime = glfwGetTime() - objRenderStartTime;

    double guiRenderStartTime = glfwGetTime();
    if (!m_cameraController.hideGUI) m_vulGUI.endFrame(commandBuffer);
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

bool Vulkano::createGlobalDescriptorSets()
{
    VkDescriptorImageInfo imageInfo[MAX_TEXTURES];
    for (size_t j = 0; j < MAX_TEXTURES; j++){
        imageInfo[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        if (j < imageCount){
            imageInfo[j].imageView = images[j]->getImageView();
            imageInfo[j].sampler = images[j]->getTextureSampler();
        } else{
            imageInfo[j].imageView = m_emptyImage.getImageView();
            imageInfo[j].sampler = m_emptyImage.getTextureSampler();
        }
    }
    for (size_t i = 0; i < VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
        VkDescriptorBufferInfo bufferInfo = m_uboBuffers[i]->descriptorInfo();
        VkDescriptorBufferInfo matBufInfo;
        if (hasScene) matBufInfo = scene.materialBuffer->descriptorInfo();
        VulDescriptorSet vulSet(*m_globalSetLayout, *m_globalPool);
        vulSet.writeBuffer(0, &bufferInfo)
            .writeImage(1, imageInfo, MAX_TEXTURES);
        if (hasScene) vulSet.writeBuffer(2, &matBufInfo);
        vulSet.build();
        if (!vulSet.hasSet()){
            fprintf(stderr, "Allocating globalDescriptorSets\n");
            return false;
        }
        m_globalDescriptorSets.push_back(std::move(vulSet));
    }

    return true;
}

bool Vulkano::createImGuiDescriptorSets()
{ 
    std::vector<uint32_t> imguiImages;
    for (uint32_t i = 0; i < imageCount; i++){
        if (!images[i]->usableByImGui) continue;
        imguiImages.push_back(i);
        for (int j = 0; j < VulSwapChain::MAX_FRAMES_IN_FLIGHT; j++){
            VkDescriptorImageInfo imageInfo;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = images[i]->getImageView();
            imageInfo.sampler = images[i]->getTextureSampler();

            VulDescriptorSet vulSet(*m_imGuiSetLayout, *m_globalPool);
            vulSet.writeImage(0, &imageInfo)
                .build();
            if (!vulSet.hasSet()){
                fprintf(stderr, "Allocating descriptorSets for imGuiImages\n");
                return false;
            }
            m_imGuiDescriptorSets.push_back(std::move(vulSet));
        }
    }

    return true;
}

void Vulkano::createNewRenderSystem(const std::vector<VkDescriptorSetLayout> &setLayouts, bool is2D, std::string vertShaderName, std::string fragShaderName)
{
    std::unique_ptr<RenderSystem> renderSystem = std::make_unique<RenderSystem>(m_vulDevice);
    if (vertShaderName != "") renderSystem->vertShaderName = vertShaderName;
    if (fragShaderName != "") renderSystem->fragShaderName = fragShaderName;
    renderSystem->init(setLayouts, m_vulRenderer.getSwapChainColorFormat(), m_vulRenderer.getSwapChainDepthFormat(), is2D);
    renderSystem->index = renderSystems.size();
    renderSystems.push_back(std::move(renderSystem));
}
        
}

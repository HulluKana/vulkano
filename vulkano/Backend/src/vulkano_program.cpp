#include"../../vulkano_program.hpp"

#include<imgui.h>
#include<imconfig.h>
#include<imgui_internal.h>
#include<imgui_impl_vulkan.h>
#include<imgui_impl_glfw.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>
#include<glm/gtc/constants.hpp>

#include<stdexcept>
#include<iostream>

#define MAX_LIGHTS 10
#define MAX_TEXTURES 10

using namespace vulB;
namespace vul{

struct GlobalUbo {
    glm::mat4 projectionView{1.0f};
    glm::vec4 camerePosition{0.0f}; //4th component is ignored

    glm::vec4 ambientLightColor{1.0f, 1.0f, 1.0f, 0.02f};

    glm::vec4 lightPosition[MAX_LIGHTS]; // 4th component is ignored
    glm::vec4 lightColor[MAX_LIGHTS];
    int numLights = 0;
};

Vulkano::Vulkano(uint32_t width, uint32_t height, std::string &name) : m_vulWindow{(int)width, (int)height, name}
{

}

Vulkano::~Vulkano()
{
    m_vulGUI.destroyImGui();
}

void Vulkano::addImages(std::vector<std::unique_ptr<VulImage>> &vulImages)
{
    for (size_t i = 0; i < vulImages.size(); i++){
        m_images.push_back(std::move(vulImages[i]));
    }
}

void Vulkano::initVulkano()
{
    if (m_images.size() == 0){
        uint8_t *data = new uint8_t[4]{255, 255, 255, 255};
        std::unique_ptr<VulImage> emptyImage = VulImage::createAsUniquePtr(m_vulDevice);
        emptyImage->createTextureFromData(data, 1, 1);
        m_images.push_back(std::move(emptyImage));
    }
    std::vector<size_t> ImGuiImages;
    for (size_t i = 0; i < m_images.size(); i++){
        if (m_images[i]->usableByImGui) ImGuiImages.push_back(i);
    }
    if (ImGuiImages.size() > 4) fprintf(stderr, "WARNING! You have %ld images enabled for ImGui. This causes a creation of quite few descriptorsets, maybe a litte too many. You may want to reduce their amount.", ImGuiImages.size());

    m_globalPool = VulDescriptorPool::Builder(m_vulDevice)
        .setMaxSets(VulSwapChain::MAX_FRAMES_IN_FLIGHT * (ImGuiImages.size() + 2))
        .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VulSwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VulSwapChain::MAX_FRAMES_IN_FLIGHT * (MAX_TEXTURES + ImGuiImages.size() + 1))
        .build();

    for (size_t i = 0; i < VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
        std::unique_ptr<VulBuffer> vulBuffer;
        vulBuffer = std::make_unique<VulBuffer>(m_vulDevice, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_vulDevice.properties.limits.minUniformBufferOffsetAlignment);
        vulBuffer->map();
        m_uboBuffers.push_back(std::move(vulBuffer));
    }

    std::unique_ptr<VulDescriptorSetLayout> globalSetLayout = VulDescriptorSetLayout::Builder(m_vulDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, MAX_TEXTURES)
        .build();

    std::unique_ptr<VulDescriptorSetLayout> ImGuiImagesSetLayout = VulDescriptorSetLayout::Builder(m_vulDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();
    
    for (size_t i = 0; i < VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
        VkDescriptorBufferInfo bufferInfo = m_uboBuffers[i]->descriptorInfo();
        VkDescriptorImageInfo imageInfo[MAX_TEXTURES];
        for (size_t j = 0; j < MAX_TEXTURES; j++){
            int imageIndex = glm::min(j, m_images.size() - 1);
            imageInfo[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo[j].imageView = m_images[imageIndex]->getImageView();
            imageInfo[j].sampler = m_images[imageIndex]->getTextureSampler();
        }
        
        VkDescriptorSet descriptorSet;
        VulDescriptorWriter(*globalSetLayout, *m_globalPool)
            .writeBuffer(0, &bufferInfo)
            .writeImage(1, imageInfo, MAX_TEXTURES)
            .build(descriptorSet);
        m_globalDescriptorSets.push_back(descriptorSet);

        for (size_t j : ImGuiImages){
            VkDescriptorImageInfo ImGuiImageInfo;
            ImGuiImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ImGuiImageInfo.imageView = m_images[j]->getImageView();
            ImGuiImageInfo.sampler = m_images[j]->getTextureSampler();
            VkDescriptorSet ImGuiImageDescriptorSet;
            VulDescriptorWriter(*ImGuiImagesSetLayout, *m_globalPool)
                .writeImage(0, &ImGuiImageInfo)
                .build(ImGuiImageDescriptorSet);

            m_images[j]->setDescriptorSet(ImGuiImageDescriptorSet);
            m_globalDescriptorSets.push_back(ImGuiImageDescriptorSet); 
        }
    }
    m_cameraObject.transform.rotation = glm::vec3(0.0f);

    std::vector<VkDescriptorSetLayout> setLayouts{globalSetLayout->getDescriptorSetLayout()};
    for (size_t i = 0; i < ImGuiImages.size(); i++){
        setLayouts.push_back(ImGuiImagesSetLayout->getDescriptorSetLayout());
    }
    m_simpleRenderSystem.init(m_vulRenderer.getSwapChainRenderPass(), setLayouts, "../vulkano/Backend/Shaders", m_vulRenderer.getSwapChainColorFormat(), m_vulRenderer.getSwapChainDepthFormat());

    vkDeviceWaitIdle(m_vulDevice.device());
    m_vulGUI.initImGui(m_vulWindow.getGLFWwindow(), m_globalPool->getDescriptorPoolReference(), m_vulRenderer, m_vulDevice);

    m_currentTime = glfwGetTime();
}

VkCommandBuffer Vulkano::startFrame()
{
    glfwPollEvents();
        
    double newTime = glfwGetTime();
    double idleStartTime = glfwGetTime();
    while (1){
        if (1.0 / (newTime - m_currentTime) < maxFps) break;
        newTime = glfwGetTime();
    }
    m_idleTime = glfwGetTime() - idleStartTime;
    m_frameTime = newTime - m_currentTime;
    m_currentTime = newTime;
    double renderPreparationStartTime = glfwGetTime();

    ImGuiIO &io = ImGui::GetIO(); (void)io;
    if (!io.WantCaptureKeyboard) m_cameraController.modifyValues(m_vulWindow.getGLFWwindow(), m_cameraObject);
    if (!io.WantCaptureMouse && !io.WantCaptureKeyboard) m_cameraController.rotate(m_vulWindow.getGLFWwindow(), m_frameTime, m_cameraObject, m_vulRenderer.getSwapChainExtent().width, m_vulRenderer.getSwapChainExtent().height);
    if (!io.WantCaptureKeyboard) m_cameraController.move(m_vulWindow.getGLFWwindow(), m_frameTime, m_cameraObject);

    float aspect = m_vulRenderer.getAspectRatio();
    m_camera.setPerspectiveProjection(cameraProperties.fovY, aspect, cameraProperties.nearPlane, cameraProperties.farPlane);
    m_camera.setViewYXZ(m_cameraObject.transform.posOffset, m_cameraObject.transform.rotation);

    if (VkCommandBuffer commandBuffer = m_vulRenderer.beginFrame()){
        int frameIndex = m_vulRenderer.getFrameIndex();

        GlobalUbo ubo{};
        ubo.projectionView = m_camera.getProjection() * m_camera.getView();
        ubo.camerePosition = glm::vec4(m_cameraObject.transform.posOffset, 0.0f);

        int lightIndex = 0;
        for (VulObject &obj : m_objects){
            if (obj.isLight && lightIndex < MAX_LIGHTS){
                ubo.lightPosition[lightIndex] = glm::vec4(obj.transform.posOffset, 1.0f);
                ubo.lightColor[lightIndex] = glm::vec4(obj.lightColor, obj.lightIntensity);
                lightIndex++;
            }
        }
        ubo.numLights = lightIndex;

        m_uboBuffers[frameIndex]->writeToBuffer(&ubo);

        m_vulRenderer.beginRendering(commandBuffer);
        if (!m_cameraController.hideGUI) m_vulGUI.startFrame();

        m_renderPreparationTime = glfwGetTime() - renderPreparationStartTime;
        return commandBuffer;
    }

    return NULL;
}

bool Vulkano::endFrame(VkCommandBuffer commandBuffer)
{
    double objRenderStartTime = glfwGetTime();
    if (m_objects.size() > 0){
        std::vector<VkDescriptorSet> descriptorSets;
        for (size_t i = (size_t)m_vulRenderer.getFrameIndex() * (m_globalDescriptorSets.size() / VulSwapChain::MAX_FRAMES_IN_FLIGHT); i < m_globalDescriptorSets.size() * (1 + m_vulRenderer.getFrameIndex()) / VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
            descriptorSets.push_back(m_globalDescriptorSets[i]);
        }
        m_simpleRenderSystem.renderObjects(m_objects, descriptorSets, commandBuffer, MAX_LIGHTS);
    }
    m_objRenderTime = glfwGetTime() - objRenderStartTime;

    double guiRenderStartTime = glfwGetTime();
    if (!m_cameraController.hideGUI) m_vulGUI.endFrame(commandBuffer);
    m_GuiRenderTime = glfwGetTime() - guiRenderStartTime;

    double renderFinishStartTime = glfwGetTime();
    m_vulRenderer.stopRendering(commandBuffer);
    m_vulRenderer.endFrame();
    vkDeviceWaitIdle(m_vulDevice.device());
    m_renderFinishingTime = glfwGetTime() - renderFinishStartTime;

    return m_vulWindow.shouldClose();
}

void Vulkano::loadObject(std::string file)
{
    std::string filePath = "../Models/" + file + ".obj";
    std::shared_ptr<VulModel> vulModel = VulModel::createModelFromFile(m_vulDevice, filePath);

    VulObject object;
    object.model = vulModel;
    object.transform.posOffset = {0.0f, 0.0f, 0.0f};
    object.transform.scale = {1.0f, 1.0f, 1.0f};
    object.color = {1.0f, 1.0f, 1.0f};

    m_objects.push_back(std::move(object));
}
        
}
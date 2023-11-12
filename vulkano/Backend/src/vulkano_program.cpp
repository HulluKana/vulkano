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

void Vulkano::initVulkano()
{
    m_globalPool = VulDescriptorPool::Builder(m_vulDevice)
        .setMaxSets(m_vulDevice.properties.limits.maxBoundDescriptorSets)
        .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VulSwapChain::MAX_FRAMES_IN_FLIGHT * MAX_UBOS)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VulSwapChain::MAX_FRAMES_IN_FLIGHT * MAX_TEXTURES * 2)
        .build();

    for (size_t i = 0; i < VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
        std::unique_ptr<VulBuffer> vulBuffer;
        vulBuffer = std::make_unique<VulBuffer>(m_vulDevice, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_vulDevice.properties.limits.minUniformBufferOffsetAlignment);
        vulBuffer->map();
        m_uboBuffers.push_back(std::move(vulBuffer));
    }

    uint8_t *data = new uint8_t[16]{255, 0, 255, 255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 0, 255, 255};
    m_emptyImage.createTextureFromData(data, 2, 2);

    m_globalSetLayout = VulDescriptorSetLayout::Builder(m_vulDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, MAX_TEXTURES)
        .build();
    updateGlobalDescriptorSets();

    m_imGuiSetLayout = VulDescriptorSetLayout::Builder(m_vulDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();
    updateImGuiDescriptorSets();

    createNewRenderSystem();

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
        if (1.0 / (newTime - m_currentTime) < settings::maxFps) break;
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
    if (settings::cameraProperties.hasPerspective) m_camera.setPerspectiveProjection(settings::cameraProperties.fovY, aspect, settings::cameraProperties.nearPlane, settings::cameraProperties.farPlane);
    else m_camera.setOrthographicProjection(settings::cameraProperties.leftPlane, settings::cameraProperties.rightPlane, settings::cameraProperties.topPlane,
                                            settings::cameraProperties.bottomPlane, settings::cameraProperties.nearPlane, settings::cameraProperties.farPlane);
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
                obj.lightIndex = lightIndex;
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
        descriptorSets.push_back(m_globalDescriptorSets[m_vulRenderer.getFrameIndex()]);

        for (size_t i = 0; i < renderSystems.size(); i++)
            renderSystems[i]->render(m_objects, descriptorSets, commandBuffer);
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

bool Vulkano::updateGlobalDescriptorSets()
{
    std::vector<VkDescriptorSet> descriptorSets;
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
        VkDescriptorSet descriptorSet;
        if (!VulDescriptorWriter(*m_globalSetLayout, *m_globalPool)
            .writeBuffer(0, &bufferInfo)
            .writeImage(1, imageInfo, MAX_TEXTURES)
            .build(descriptorSet)){
            fprintf(stderr, "Allocating globalDescriptorSets failed. Old descriptorSets remain in use and unchanged\n");
        }
        descriptorSets.push_back(descriptorSet);
    }

    // I have to clear the previous sets after new sets have been created and not after, because allocating set could fail and clearing previous sets before that would leave the set vector empty
    if (m_globalDescriptorSets.size() > 0){
        vkFreeDescriptorSets(m_vulDevice.device(), m_globalPool->getDescriptorPoolReference(), static_cast<uint32_t>(m_globalDescriptorSets.size()), m_globalDescriptorSets.data());
        m_globalDescriptorSets.clear();
    }
    for (VkDescriptorSet &set : descriptorSets)
        m_globalDescriptorSets.push_back(set);

    return true;
}

bool Vulkano::updateImGuiDescriptorSets()
{ 
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<uint32_t> imguiImages;
    for (uint32_t i = 0; i < imageCount; i++){
        if (!images[i]->usableByImGui) continue;
        imguiImages.push_back(i);
        for (int j = 0; j < VulSwapChain::MAX_FRAMES_IN_FLIGHT; j++){
            VkDescriptorImageInfo imageInfo;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = images[i]->getImageView();
            imageInfo.sampler = images[i]->getTextureSampler();

            VkDescriptorSet descriptorSet;
            if (!VulDescriptorWriter(*m_imGuiSetLayout, *m_globalPool)
                .writeImage(0, &imageInfo)
                .build(descriptorSet)){
                fprintf(stderr, "Allocating descriptorSets for imGuiImages failed. Old descriptorSets remain in use and unchanged\n");
                return false;
            }
            descriptorSets.push_back(descriptorSet);
        }
    }

    // I have to clear the previous sets after new sets have been created and not after, because allocating set could fail and clearing previous sets before that would leave the set vector empty
    if (m_imGuiDescriptorSets.size() > 0){
        vkFreeDescriptorSets(m_vulDevice.device(), m_globalPool->getDescriptorPoolReference(), static_cast<uint32_t>(m_imGuiDescriptorSets.size()), m_imGuiDescriptorSets.data());
        m_imGuiDescriptorSets.clear();
    }
    for (int i = 0; i < static_cast<int>(descriptorSets.size()); i++){
        m_imGuiDescriptorSets.push_back(descriptorSets[i]);
        if (i % VulSwapChain::MAX_FRAMES_IN_FLIGHT == 0)
            images[i / VulSwapChain::MAX_FRAMES_IN_FLIGHT]->setDescriptorSet(descriptorSets[i]);
    }

    return true;
}

void Vulkano::createNewRenderSystem(std::string vertShaderName, std::string fragShaderName)
{
    std::vector<VkDescriptorSetLayout> setLayouts{m_globalSetLayout->getDescriptorSetLayout(), m_imGuiSetLayout->getDescriptorSetLayout()};
    std::unique_ptr<RenderSystem> renderSystem = std::make_unique<RenderSystem>(m_vulDevice);
    if (vertShaderName != "") renderSystem->vertShaderName = vertShaderName;
    if (fragShaderName != "") renderSystem->fragShaderName = fragShaderName;
    renderSystem->init(setLayouts, m_vulRenderer.getSwapChainColorFormat(), m_vulRenderer.getSwapChainDepthFormat());
    renderSystem->index = renderSystems.size();
    renderSystems.push_back(std::move(renderSystem));
}
        
}
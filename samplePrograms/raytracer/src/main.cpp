#include "vul_buffer.hpp"
#include "vul_image.hpp"
#include "vul_pipeline.hpp"
#include "vul_renderer.hpp"
#include "vul_rt_pipeline.hpp"
#include "vul_swap_chain.hpp"
#include <glm/detail/qualifier.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/matrix.hpp>
#include <memory>
#include <vulkan/vulkan_core.h>
#include<vulkano_program.hpp>
#include<host_device.hpp>
#include<vul_debug_tools.hpp>
#include <vul_acceleration_structure.hpp>
#include <iostream>

#include<imgui.h>

void GuiStuff(vul::Vulkano &vulkano, float ownStuffTime) {
    ImGui::Begin("Performance");
    ImGui::DragFloat("Max FPS", &vul::settings::maxFps, vul::settings::maxFps / 30.0f, 3.0f, 10'000.0f);
    ImGui::Text("Fps: %f\nTotal frame time: %fms\nIdle time %fms\nOwn stuff: %fms",
            1.0f / vulkano.getFrameTime(), vulkano.getFrameTime() * 1000.0f,
            vulkano.getIdleTime() * 1000.0f, ownStuffTime * 1000.0f);
    ImGui::End();
}

int main() {
    vul::settings::deviceInitConfig.enableRaytracingSupport = true;
    vul::Vulkano vulkano(2560, 1440, "Vulkano");
    vulkano.loadScene("../Models/sponza.gltf");
    vulkano.createSquare(0.0f, 0.0f, 1.0f, 1.0f);
    vulkano.initVulkano();
    vul::settings::maxFps = 60.0f;

    vul::VulAs as(vulkano.getVulDevice(), vulkano.scene);
    std::array<std::unique_ptr<vul::VulImage>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> rtImgs;
    std::array<std::unique_ptr<vulB::VulBuffer>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
    std::array<vul::Vulkano::descSetReturnVal, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> dsrvs;
    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        rtImgs[i] = std::make_unique<vul::VulImage>(vulkano.getVulDevice());
        rtImgs[i]->keepEmpty(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height, 4, 16);
        VkCommandBuffer cmdBuf = vulkano.getVulDevice().beginSingleTimeCommands();
        rtImgs[i]->createImageLowLevel(false, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TYPE_2D, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_STORAGE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_LINEAR, cmdBuf);
        vulkano.getVulDevice().endSingleTimeCommands(cmdBuf);

        ubos[i] = std::make_unique<vulB::VulBuffer>(vulkano.getVulDevice());
        ubos[i]->keepEmpty(sizeof(GlobalUbo), 1);
        ubos[i]->createBuffer(false, vulB::VulBuffer::usage_ubo);

        std::vector<vul::Vulkano::Descriptor> descriptors;
        vul::Vulkano::Descriptor desc{};
        desc.count = 1;

        desc.type = vul::Vulkano::DescriptorType::storageImage;
        desc.stages = {vul::Vulkano::ShaderStage::rgen, vul::Vulkano::ShaderStage::frag};
        desc.content = rtImgs[i].get();
        descriptors.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::ubo;
        desc.stages = {vul::Vulkano::ShaderStage::rgen, vul::Vulkano::ShaderStage::rmiss, vul::Vulkano::ShaderStage::rchit};
        desc.content = ubos[i].get();
        descriptors.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::ssbo;
        desc.stages = {vul::Vulkano::ShaderStage::rchit};
        desc.content = vulkano.scene.indexBuffer.get();
        descriptors.push_back(desc);
        desc.content = vulkano.scene.vertexBuffer.get();
        descriptors.push_back(desc);
        desc.content = vulkano.scene.normalBuffer.get();
        descriptors.push_back(desc);
        desc.content = vulkano.scene.tangentBuffer.get();
        descriptors.push_back(desc);
        desc.content = vulkano.scene.uvBuffer.get();
        descriptors.push_back(desc);
        desc.content = vulkano.scene.materialBuffer.get();
        descriptors.push_back(desc);
        desc.content = vulkano.scene.primInfoBuffer.get();
        descriptors.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::spCombinedTexSampler;
        desc.content = vulkano.scene.images.data();
        desc.count = vulkano.scene.images.size();
        descriptors.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::accelerationStructure;
        desc.stages = {vul::Vulkano::ShaderStage::rgen};
        desc.count = 1;
        desc.content = &as;
        descriptors.push_back(desc);

        dsrvs[i] = vulkano.createDescriptorSet(descriptors);
    }

    vulB::VulPipeline::PipelineConfigInfo pipConf{};
    pipConf.attributeDescriptions = {{0, 0, VK_FORMAT_R32G32_SFLOAT, 0}, {1, 1, VK_FORMAT_R32G32_SFLOAT, 0}};
    pipConf.bindingDescriptions = {{0, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}, {1, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}};
    pipConf.colorAttachmentFormats = {vulkano.vulRenderer.getSwapChainColorFormat()};
    pipConf.depthAttachmentFormat = vulkano.vulRenderer.getDepthFormat();
    pipConf.setLayouts = {dsrvs[0].set->getLayout()->getDescriptorSetLayout()};
    pipConf.enableColorBlending = false;
    pipConf.cullMode = VK_CULL_MODE_NONE;

    vul::Vulkano::RenderData renderData{};
    renderData.is3d = false;
    renderData.depthImageMode = vulB::VulRenderer::DepthImageMode::noDepthImage;
    renderData.swapChainImageMode = vulB::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent;
    renderData.sampleFromDepth = false;
    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) renderData.descriptorSets[i].push_back(std::move(dsrvs[i].set));
    renderData.pipeline = std::make_shared<vulB::VulPipeline>(vulkano.getVulDevice(), "../bin/raytrace.vert.spv", "../bin/raytrace.frag.spv", pipConf);

    vul::VulRtPipeline rtPipeline(vulkano.getVulDevice(), "../bin/raytrace.rgen.spv", {"../bin/raytrace.rmiss.spv"}, {"../bin/raytrace.rchit.spv"},
            {renderData.descriptorSets[0][0]->getLayout()->getDescriptorSetLayout()});

    vulkano.renderDatas.push_back(renderData);

    bool stop = false;
    float ownStuffTime = 0.0f;
    while (!stop) {
        VkCommandBuffer commandBuffer = vulkano.startFrame();
        if (commandBuffer == nullptr) continue;
        double ownStuffStartTime = glfwGetTime();

        if (vulkano.shouldShowGUI()) GuiStuff(vulkano, ownStuffTime);

        GlobalUbo ubo{};
        ubo.inverseViewMatrix = glm::inverse(vulkano.camera.getView());
        ubo.inverseProjectionMatrix = glm::inverse(vulkano.camera.getProjection());
        ubo.numLights = 0;
        ubo.cameraPosition = glm::vec4(vulkano.cameraTransform.pos, 0.0f);
        ubo.ambientLightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.3f);
        ubo.numLights = vulkano.scene.lights.size();
        for (int i = 0; i < std::min(static_cast<int>(vulkano.scene.lights.size()), MAX_LIGHTS); i++){
            ubo.lightPositions[i] = glm::vec4(vulkano.scene.lights[i].position, 69.0f);
            ubo.lightColors[i] = glm::vec4(vulkano.scene.lights[i].color, vulkano.scene.lights[i].intensity);
        }
        ubos[vulkano.vulRenderer.getFrameIndex()]->writeData(&ubo, sizeof(GlobalUbo), 0);

        std::vector<VkDescriptorSet> descSets = {vulkano.renderDatas[0].descriptorSets[(vulkano.vulRenderer.getFrameIndex() + 1) % vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT]
            [0]->getSet()};
        rtPipeline.traceRays(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height, 0, nullptr, descSets, commandBuffer);
            
        ownStuffTime = glfwGetTime() - ownStuffStartTime;
        stop = vulkano.endFrame(commandBuffer);

        if (vulkano.vulRenderer.wasSwapChainRecreated()) {
            for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
                VkCommandBuffer cmdBuf = vulkano.getVulDevice().beginSingleTimeCommands();
                rtImgs[i] = std::make_unique<vul::VulImage>(vulkano.getVulDevice());
                rtImgs[i]->keepEmpty(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height, 4, 16);
                rtImgs[i]->createImageLowLevel(false, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TYPE_2D, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_STORAGE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_LINEAR, cmdBuf);
                vulkano.getVulDevice().endSingleTimeCommands(cmdBuf);


                vulkano.renderDatas[0].descriptorSets[i][0]->descriptorInfos[0].imageInfos[0].imageView = rtImgs[i]->getImageView();
                vulkano.renderDatas[0].descriptorSets[i][0]->update();
            }
        }
    }
    vulkano.letVulkanoFinish();

    return 0;
}

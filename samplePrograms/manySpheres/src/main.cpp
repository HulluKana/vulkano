#include "vul_settings.hpp"
#include "vul_swap_chain.hpp"
#include "vul_transform.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>
#include<vulkano_program.hpp>
#include <host_device.hpp>

#include<imgui.h>
#include <iostream>

void GuiStuff(vul::Vulkano &vulkano) {
    ImGui::Begin("Performance");
    ImGui::DragFloat("Max FPS", &vul::settings::maxFps, vul::settings::maxFps / 30.0f, 3.0f, 10'000.0f);
    ImGui::Text("Fps: %f\nTotal frame time: %fms\nIdle time %fms",
            1.0f / vulkano.getFrameTime(), vulkano.getFrameTime() * 1000.0f,
            vulkano.getIdleTime() * 1000.0f);
    ImGui::End();
}

int main() {
    constexpr int VOLUME_LEN = 20;
    vul::Vulkano vulkano(2560, 1440, "Vulkano");
    vul::settings::maxFps = 60.0f;
    vul::settings::cameraProperties.farPlane = 500.0f;
    vulkano.cameraController.baseMoveSpeed *= 2.5f;
    vulkano.cameraController.speedChanger *= 5.0f;

    std::vector<glm::mat4> transformMats;
    for (int x = -VOLUME_LEN / 2; x < VOLUME_LEN / 2; x++) {
        for (int y = -VOLUME_LEN / 2; y < VOLUME_LEN / 2; y++) {
            for (int z = -VOLUME_LEN / 2; z < VOLUME_LEN / 2; z++) {
                vul::transform3D transform{};
                transform.pos = glm::vec3(x, y, z);
                transform.rot = glm::vec3(0.0f);
                transform.scale = glm::vec3(0.3f);
                transformMats.emplace_back(transform.transformMat());
            }
        }
    }
    std::shared_ptr<vulB::VulBuffer> transformBuf = std::make_shared<vulB::VulBuffer>(vulkano.getVulDevice());
    transformBuf->loadVector(transformMats);
    transformBuf->createBuffer(true, static_cast<vulB::VulBuffer::Usage>(vulB::VulBuffer::usage_ssbo | vulB::VulBuffer::usage_transferDst));

    //vulkano.scene.loadCubes({{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}});
    vulkano.scene.loadSpheres({{{0.0f, 0.0f, 0.0f}, 1.0f, 3}});
    vulkano.hasScene = true;

    vulB::VulPipeline::DrawData drawData;
    drawData.firstIndex = vulkano.scene.meshes[0].firstIndex;
    drawData.indexCount = vulkano.scene.meshes[0].indexCount;
    drawData.vertexOffset = vulkano.scene.meshes[0].vertexOffset;
    drawData.instanceCount = transformMats.size();
    drawData.firstInstance = 0;
    drawData.pPushData = nullptr;
    drawData.pushDataSize = 0;

    std::array<std::unique_ptr<vulB::VulBuffer>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
    std::array<std::shared_ptr<vulB::VulDescriptorSet>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> descSets;
    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        ubos[i] = std::make_unique<vulB::VulBuffer>(vulkano.getVulDevice());
        ubos[i]->keepEmpty(sizeof(Ubo), 1);
        ubos[i]->createBuffer(false, static_cast<vulB::VulBuffer::Usage>(vulB::VulBuffer::usage_ubo | vulB::VulBuffer::usage_transferDst));

        std::vector<vul::Vulkano::Descriptor> descs;
        vul::Vulkano::Descriptor desc;
        desc.stages = {vul::Vulkano::ShaderStage::vert};
        desc.count = 1;

        desc.type = vul::Vulkano::DescriptorType::ubo;
        desc.content = ubos[i].get();
        descs.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::ssbo;
        desc.content = transformBuf.get();
        descs.push_back(desc);

        descSets[i] = vulkano.createDescriptorSet(descs);
    }

    vulB::VulPipeline::PipelineConfigInfo pipelineConfig{};
    pipelineConfig.colorAttachmentFormats = {vulkano.vulRenderer.getSwapChainColorFormat()};
    pipelineConfig.depthAttachmentFormat = vulkano.vulRenderer.getDepthFormat();
    pipelineConfig.enableColorBlending = false;
    pipelineConfig.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineConfig.attributeDescriptions = {VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}};
    pipelineConfig.bindingDescriptions = {VkVertexInputBindingDescription{0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX}};
    pipelineConfig.setLayouts = {descSets[0]->getLayout()->getDescriptorSetLayout()};

    vul::Vulkano::RenderData renderData{};
    renderData.is3d = true;
    renderData.swapChainImageMode = vulB::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent;
    renderData.depthImageMode = vulB::VulRenderer::DepthImageMode::clearPreviousStoreCurrent;
    renderData.sampleFromDepth = false;
    renderData.pipeline = std::make_shared<vulB::VulPipeline>(vulkano.getVulDevice(), "../bin/default.vert.spv", "../bin/default.frag.spv", pipelineConfig);
    renderData.drawDatas = {drawData};
    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) renderData.descriptorSets[i].push_back(descSets[i]);
    vulkano.renderDatas.push_back(renderData);

    bool stop = false;
    bool createdCamStuff = false;
    while (!stop) {
        VkCommandBuffer commandBuffer = vulkano.startFrame();
        if (commandBuffer == nullptr) continue;

        if (!createdCamStuff) {
            Ubo ubo{};
            ubo.viewMatrix = vulkano.camera.getView();
            ubo.projectionMatrix = vulkano.camera.getProjection();
            for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) ubos[i]->writeData(&ubo, sizeof(ubo), 0);
            createdCamStuff = true;
        }

        if (vulkano.shouldShowGUI()) GuiStuff(vulkano);

        glm::mat4 viewMatrix = vulkano.camera.getView();
        ubos[vulkano.getFrameIdx()]->writeData(&viewMatrix, sizeof(viewMatrix), 0);
        stop = vulkano.endFrame(commandBuffer);
    }
    vulkano.letVulkanoFinish();

    return 0;
}

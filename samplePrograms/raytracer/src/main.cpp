#include "vul_scene.hpp"
#include "vul_transform.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <vul_rt_pipeline.hpp>
#include<vulkano_program.hpp>
#include<host_device.hpp>
#include<vul_debug_tools.hpp>
#include <vul_acceleration_structure.hpp>

#include<imgui.h>

void GuiStuff(vul::Vulkano &vulkano) {
    ImGui::Begin("Performance");
    ImGui::DragFloat("Max FPS", &vul::settings::maxFps, vul::settings::maxFps / 30.0f, 3.0f, 10'000.0f);
    ImGui::Text("Fps: %f\nTotal frame time: %fms\nIdle time %fms",
            1.0f / vulkano.getFrameTime(), vulkano.getFrameTime() * 1000.0f,
            vulkano.getIdleTime() * 1000.0f);
    ImGui::End();
}

std::unique_ptr<vulB::VulDescriptorSet> createRtDescSet(const vul::Vulkano &vulkano, const vul::VulAs &as, const std::unique_ptr<vul::VulImage> &rtImg, const std::unique_ptr<vulB::VulBuffer> &ubo, const std::unique_ptr<vul::VulImage> &enviromentMap)
{
    std::vector<vul::Vulkano::Descriptor> descriptors;
    vul::Vulkano::Descriptor desc{};
    desc.count = 1;

    desc.type = vul::Vulkano::DescriptorType::storageImage;
    desc.stages = {vul::Vulkano::ShaderStage::rgen, vul::Vulkano::ShaderStage::frag};
    desc.content = rtImg.get();
    descriptors.push_back(desc);

    desc.type = vul::Vulkano::DescriptorType::ubo;
    desc.stages = {vul::Vulkano::ShaderStage::rgen, vul::Vulkano::ShaderStage::rmiss, vul::Vulkano::ShaderStage::rchit};
    desc.content = ubo.get();
    descriptors.push_back(desc);

    desc.type = vul::Vulkano::DescriptorType::ssbo;
    desc.stages = {vul::Vulkano::ShaderStage::rchit, vul::Vulkano::ShaderStage::rahit};
    desc.content = vulkano.scene.indexBuffer.get();
    descriptors.push_back(desc);
    desc.content = vulkano.scene.vertexBuffer.get();
    descriptors.push_back(desc);

    desc.stages = {vul::Vulkano::ShaderStage::rchit};
    desc.content = vulkano.scene.indexBuffer.get();
    desc.content = vulkano.scene.normalBuffer.get();
    descriptors.push_back(desc);
    desc.content = vulkano.scene.tangentBuffer.get();
    descriptors.push_back(desc);

    desc.stages = {vul::Vulkano::ShaderStage::rchit, vul::Vulkano::ShaderStage::rahit};
    desc.content = vulkano.scene.indexBuffer.get();
    desc.content = vulkano.scene.uvBuffer.get();
    descriptors.push_back(desc);
    desc.content = vulkano.scene.materialBuffer.get();
    descriptors.push_back(desc);
    desc.content = vulkano.scene.primInfoBuffer.get();
    descriptors.push_back(desc);

    desc.type = vul::Vulkano::DescriptorType::spCombinedImgSampler;
    desc.content = vulkano.scene.images.data();
    desc.count = vulkano.scene.images.size();
    descriptors.push_back(desc);

    desc.type = vul::Vulkano::DescriptorType::combinedImgSampler;
    desc.stages = {vul::Vulkano::ShaderStage::rmiss};
    desc.content = enviromentMap.get();
    desc.count = 1;
    descriptors.push_back(desc);

    desc.type = vul::Vulkano::DescriptorType::accelerationStructure;
    desc.stages = {vul::Vulkano::ShaderStage::rgen, vul::Vulkano::ShaderStage::rchit};
    desc.count = 1;
    desc.content = &as;
    descriptors.push_back(desc);

    return vulkano.createDescriptorSet(descriptors);
}

vul::Vulkano::RenderData createRenderData(const vul::Vulkano &vulkano, const vulB::VulDevice &device, const std::array<std::shared_ptr<vulB::VulDescriptorSet>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> &descSets)
{
    vulB::VulPipeline::PipelineConfigInfo pipConf{};
    pipConf.attributeDescriptions = {{0, 0, VK_FORMAT_R32G32_SFLOAT, 0}, {1, 1, VK_FORMAT_R32G32_SFLOAT, 0}};
    pipConf.bindingDescriptions = {{0, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}, {1, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}};
    pipConf.colorAttachmentFormats = {vulkano.vulRenderer.getSwapChainColorFormat()};
    pipConf.depthAttachmentFormat = vulkano.vulRenderer.getDepthFormat();
    pipConf.setLayouts = {descSets[0]->getLayout()->getDescriptorSetLayout()};
    pipConf.enableColorBlending = false;
    pipConf.cullMode = VK_CULL_MODE_NONE;

    vul::Vulkano::RenderData renderData{};
    renderData.is3d = false;
    renderData.depthImageMode = vulB::VulRenderer::DepthImageMode::noDepthImage;
    renderData.swapChainImageMode = vulB::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent;
    renderData.sampleFromDepth = false;
    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) renderData.descriptorSets[i].push_back(descSets[i]);
    renderData.pipeline = std::make_shared<vulB::VulPipeline>(device, "../bin/raytrace.vert.spv", "../bin/raytrace.frag.spv", pipConf);

    return renderData;
}

void resizeRtImgs(vul::Vulkano &vulkano, std::array<std::unique_ptr<vul::VulImage>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> &rtImgs)
{
    VkCommandBuffer cmdBuf = vulkano.getVulDevice().beginSingleTimeCommands();
    for (size_t i = 0; i < rtImgs.size(); i++) {
        rtImgs[i] = std::make_unique<vul::VulImage>(vulkano.getVulDevice());
        rtImgs[i]->keepRegularRaw2d32bitRgbaEmpty(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height);
        rtImgs[i]->createDefaultImage(vul::VulImage::ImageType::storage2d, cmdBuf);
    }
    vulkano.getVulDevice().endSingleTimeCommands(cmdBuf);

    for (size_t i = 0; i < rtImgs.size(); i++) {
        vulkano.renderDatas[0].descriptorSets[i][0]->descriptorInfos[0].imageInfos[0].imageView = rtImgs[i]->getImageView();
        vulkano.renderDatas[0].descriptorSets[i][0]->update();
    }
}

void updateUbo(const vul::Vulkano &vulkano, std::unique_ptr<vulB::VulBuffer> &ubo)
{
    GlobalUbo uboData{};
    uboData.inverseViewMatrix = glm::inverse(vulkano.camera.getView());
    uboData.inverseProjectionMatrix = glm::inverse(vulkano.camera.getProjection());
    uboData.cameraPosition = glm::vec4(vulkano.cameraTransform.pos, 0.0f);

    uboData.numLights = std::min(static_cast<int>(vulkano.scene.lights.size()), MAX_LIGHTS);
    for (int i = 0; i < uboData.numLights; i++){
        uboData.lightPositions[i] = glm::vec4(vulkano.scene.lights[i].position, vulkano.scene.lights[i].range);
        uboData.lightColors[i] = glm::vec4(vulkano.scene.lights[i].color, vulkano.scene.lights[i].intensity);
    }
    uboData.ambientLightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.3f);

    // Pixel spread angle is from equation 30 from
    // https://media.contentapi.ea.com/content/dam/ea/seed/presentations/2019-ray-tracing-gems-chapter-20-akenine-moller-et-al.pdf
    uboData.pixelSpreadAngle = atan((2.0f * tan(vul::settings::cameraProperties.fovY / 2.0f)) / static_cast<float>(vulkano.getSwapChainExtent().height));
    uboData.padding1 = 69;
    uboData.padding2 = 420;

    ubo->writeData(&uboData, sizeof(GlobalUbo), 0);
}

int main() {
    vul::settings::deviceInitConfig.enableRaytracingSupport = true;
    vul::Vulkano vulkano(2560, 1440, "Vulkano");
    vulkano.loadScene("../Models/sponza/sponza.gltf", "../Models/sponza/", {});
    vulkano.createSquare(0.0f, 0.0f, 1.0f, 1.0f);
    vul::settings::maxFps = 60.0f;

    std::unique_ptr<vul::VulImage> enviromentMap = vul::VulImage::createDefaultWholeImageAllInOneSingleTime(vulkano.getVulDevice(),
            "../enviromentMaps/sunsetCube.exr", {}, true, vul::VulImage::InputDataType::exrFile, vul::VulImage::ImageType::hdrCube);

    std::vector<vul::VulAs::AsNode> asNodes(vulkano.scene.nodes.size());
    for (size_t i = 0; i < vulkano.scene.nodes.size(); i++) asNodes[i] = {.nodeIndex = static_cast<uint32_t>(i), .blasIndex = 0};
    vul::VulAs as(vulkano.getVulDevice());
    as.loadScene(vulkano.scene, asNodes, {vul::VulAs::InstanceInfo{.blasIdx = 0, .customIndex = 0,
            .shaderBindingTableRecordOffset = 0, .transform = vul::transform3D{}.transformMat()}}, false);

    std::array<std::unique_ptr<vul::VulImage>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> rtImgs;
    std::array<std::unique_ptr<vulB::VulBuffer>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
    std::array<std::shared_ptr<vulB::VulDescriptorSet>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> descSets;
    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        rtImgs[i] = std::make_unique<vul::VulImage>(vulkano.getVulDevice());
        rtImgs[i]->keepRegularRaw2d32bitRgbaEmpty(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height);
        VkCommandBuffer cmdBuf = vulkano.getVulDevice().beginSingleTimeCommands();
        rtImgs[i]->createDefaultImage(vul::VulImage::ImageType::storage2d, cmdBuf);
        vulkano.getVulDevice().endSingleTimeCommands(cmdBuf);

        ubos[i] = std::make_unique<vulB::VulBuffer>(vulkano.getVulDevice());
        ubos[i]->keepEmpty(sizeof(GlobalUbo), 1);
        ubos[i]->createBuffer(false, vulB::VulBuffer::usage_ubo);

        descSets[i] = createRtDescSet(vulkano, as, rtImgs[i], ubos[i], enviromentMap);
    }

    vulkano.renderDatas.push_back(createRenderData(vulkano, vulkano.getVulDevice(), descSets));
    vul::VulRtPipeline rtPipeline(vulkano.getVulDevice(), "../bin/raytrace.rgen.spv", {"../bin/raytrace.rmiss.spv", "../bin/raytraceShadow.rmiss.spv"},
            {"../bin/raytrace.rchit.spv"}, {"../bin/raytraceShadow.rahit.spv"}, {}, {{0, 0, -1}},
            {vulkano.renderDatas[0].descriptorSets[0][0]->getLayout()->getDescriptorSetLayout()});

    bool stop = false;
    while (!stop) {
        VkCommandBuffer commandBuffer = vulkano.startFrame();
        if (commandBuffer == nullptr) continue;
        int frameIdx = vulkano.vulRenderer.getFrameIndex();

        if (vulkano.shouldShowGUI()) GuiStuff(vulkano);
        updateUbo(vulkano, ubos[frameIdx]);

        std::vector<VkDescriptorSet> descSets = {vulkano.renderDatas[0].descriptorSets[(frameIdx + 1) % vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT]
            [0]->getSet()};
        rtPipeline.traceRays(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height, 0, nullptr, descSets, commandBuffer);
        stop = vulkano.endFrame(commandBuffer);

        if (vulkano.vulRenderer.wasSwapChainRecreated()) resizeRtImgs(vulkano, rtImgs);
    }
    vulkano.letVulkanoFinish();

    return 0;
}

#include "vul_acceleration_structure.hpp"
#include "vul_rt_pipeline.hpp"
#include <algorithm>
#include <cstddef>
#include<vulkano_program.hpp>
#include <host_device.hpp>

#include<imgui.h>
#include <iostream>
#include <random>

void GuiStuff(vul::Vulkano &vulkano) {
    ImGui::Begin("Performance");
    ImGui::DragFloat("Max FPS", &vul::settings::maxFps, vul::settings::maxFps / 30.0f, 3.0f, 10'000.0f);
    ImGui::Text("Fps: %f\nTotal frame time: %fms\nIdle time %fms",
            1.0f / vulkano.getFrameTime(), vulkano.getFrameTime() * 1000.0f,
            vulkano.getIdleTime() * 1000.0f);
    ImGui::End();
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

int main() {
    vul::settings::deviceInitConfig.enableRaytracingSupport = true;
    vul::Vulkano vulkano(2560, 1440, "Vulkano");
    vul::settings::maxFps = 60.0f;
    vul::settings::cameraProperties.farPlane = 500.0f;
    vulkano.cameraController.baseMoveSpeed *= 2.5f;
    vulkano.cameraController.speedChanger *= 5.0f;

    std::random_device dev{};
    std::mt19937 rng{dev()};
    std::uniform_real_distribution<> dist{-0.5, 0.5};

    /*
    std::vector<ObjData> objDatas;
    constexpr int VOLUME_LEN = 175;
    for (int x = -VOLUME_LEN / 2; x < VOLUME_LEN / 2; x++) {
        for (int y = -VOLUME_LEN / 2; y < VOLUME_LEN / 2; y++) {
            for (int z = -VOLUME_LEN / 2; z < VOLUME_LEN / 2; z++) {
                glm::vec3 pos = glm::vec3(x, y, z);
                pos += glm::vec3(dist(rng), dist(rng), dist(rng));

                const glm::vec3 offset = pos - glm::vec3(-static_cast<float>(VOLUME_LEN) / 2.0f); 
                const glm::vec3 color = offset / static_cast<float>(VOLUME_LEN);

                objDatas.emplace_back(ObjData{glm::vec4{pos, 1.0f}, glm::vec4{color, 1.0f}});
            }
        }
    }
    std::shared_ptr<vulB::VulBuffer> objDataBuf = std::make_shared<vulB::VulBuffer>(vulkano.getVulDevice());
    objDataBuf->loadVector(objDatas);
    objDataBuf->createBuffer(true, static_cast<vulB::VulBuffer::Usage>(vulB::VulBuffer::usage_ssbo | vulB::VulBuffer::usage_transferDst));

    vulkano.scene.loadCubes({{{0.0f, 0.0f, 0.0f}, {0.3f, 0.3f, 0.3f}}});
    //vulkano.scene.loadSpheres({{{0.0f, 0.0f, 0.0f}, 0.3f, 3}});
    vulkano.hasScene = true;

    vulB::VulPipeline::DrawData drawData;
    drawData.firstIndex = vulkano.scene.meshes[0].firstIndex;
    drawData.indexCount = vulkano.scene.meshes[0].indexCount;
    drawData.vertexOffset = vulkano.scene.meshes[0].vertexOffset;
    drawData.instanceCount = objDatas.size();
    drawData.firstInstance = 0;
    drawData.pPushData = nullptr;
    drawData.pushDataSize = 0;

    std::array<std::unique_ptr<vulB::VulBuffer>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
    std::array<std::shared_ptr<vulB::VulDescriptorSet>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> descSets;
    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        ubos[i] = std::make_unique<vulB::VulBuffer>(vulkano.getVulDevice());
        ubos[i]->keepEmpty(sizeof(RasUbo), 1);
        ubos[i]->createBuffer(false, static_cast<vulB::VulBuffer::Usage>(vulB::VulBuffer::usage_ubo | vulB::VulBuffer::usage_transferDst));

        std::vector<vul::Vulkano::Descriptor> descs;
        vul::Vulkano::Descriptor desc;
        desc.count = 1;

        desc.type = vul::Vulkano::DescriptorType::ubo;
        desc.stages = {vul::Vulkano::ShaderStage::vert};
        desc.content = ubos[i].get();
        descs.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::ssbo;
        desc.stages = {vul::Vulkano::ShaderStage::vert, vul::Vulkano::ShaderStage::frag};
        desc.content = objDataBuf.get();
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
    */

    vulkano.createSquare(0.0f, 0.0f, 1.0f, 1.0f);
    std::vector<vul::VulAs::Aabb> aabbs;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec4> spheres;
    constexpr int VOLUME_LEN = 256;
    for (int x = -VOLUME_LEN / 2; x < VOLUME_LEN / 2; x++) {
        for (int y = -VOLUME_LEN / 2; y < VOLUME_LEN / 2; y++) {
            for (int z = -VOLUME_LEN / 2; z < VOLUME_LEN / 2; z++) {
                glm::vec3 pos = glm::vec3(x, y, z);
                pos += glm::vec3(dist(rng), dist(rng), dist(rng));

                const glm::vec3 offset = pos - glm::vec3(-static_cast<float>(VOLUME_LEN) / 2.0f); 
                const glm::vec3 color = offset / static_cast<float>(VOLUME_LEN);

                aabbs.emplace_back(vul::VulAs::Aabb{pos - glm::vec3(0.3f), pos + glm::vec3(0.3f)});
                colors.push_back(glm::vec4(color, 1.0f));
                spheres.emplace_back(glm::vec4{pos, 0.3f});
            }
        }
    }
    std::unique_ptr<vulB::VulBuffer> aabbBuf = std::make_unique<vulB::VulBuffer>(vulkano.getVulDevice());
    aabbBuf->loadVector(aabbs);
    aabbBuf->createBuffer(true, static_cast<vulB::VulBuffer::Usage>(vulB::VulBuffer::usage_accelerationStructureBuildRead |
                vulB::VulBuffer::usage_getAddress | vulB::VulBuffer::usage_transferDst));

    std::unique_ptr<vulB::VulBuffer> colorBuf = std::make_unique<vulB::VulBuffer>(vulkano.getVulDevice());
    colorBuf->loadVector(colors);
    colorBuf->createBuffer(true, static_cast<vulB::VulBuffer::Usage>(vulB::VulBuffer::usage_ssbo | vulB::VulBuffer::usage_transferDst));

    std::unique_ptr<vulB::VulBuffer> spheresBuf = std::make_unique<vulB::VulBuffer>(vulkano.getVulDevice());
    spheresBuf->loadVector(spheres);
    spheresBuf->createBuffer(true, static_cast<vulB::VulBuffer::Usage>(vulB::VulBuffer::usage_ssbo | vulB::VulBuffer::usage_transferDst));

    vul::VulAs as(vulkano.getVulDevice());
    as.loadAabbs(aabbBuf);
    std::array<std::unique_ptr<vul::VulImage>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> rtImgs;
    std::array<std::unique_ptr<vulB::VulBuffer>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
    std::array<std::shared_ptr<vulB::VulDescriptorSet>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> descSets;
    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        rtImgs[i] = std::make_unique<vul::VulImage>(vulkano.getVulDevice());
        rtImgs[i]->keepRegularRaw2d32bitRgbaEmpty(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height);
        rtImgs[i]->createDefaultImageSingleTime(vul::VulImage::ImageType::storage2d);

        ubos[i] = std::make_unique<vulB::VulBuffer>(vulkano.getVulDevice());
        ubos[i]->keepEmpty(sizeof(RtUbo), 1);
        ubos[i]->createBuffer(false, static_cast<vulB::VulBuffer::Usage>(vulB::VulBuffer::usage_ubo | vulB::VulBuffer::usage_transferDst));

        std::vector<vul::Vulkano::Descriptor> descriptors;
        vul::Vulkano::Descriptor desc{};
        desc.count = 1;

        desc.type = vul::Vulkano::DescriptorType::storageImage;
        desc.stages = {vul::Vulkano::ShaderStage::rgen, vul::Vulkano::ShaderStage::frag};
        desc.content = rtImgs[i].get();
        descriptors.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::accelerationStructure;
        desc.stages = {vul::Vulkano::ShaderStage::rgen, vul::Vulkano::ShaderStage::rchit};
        desc.count = 1;
        desc.content = &as;
        descriptors.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::ssbo;
        desc.stages = {vul::Vulkano::ShaderStage::rchit};
        desc.content = colorBuf.get();
        descriptors.push_back(desc);

        desc.stages = {vul::Vulkano::ShaderStage::rint};
        desc.content = spheresBuf.get();
        descriptors.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::ubo;
        desc.stages = {vul::Vulkano::ShaderStage::vert, vul::Vulkano::ShaderStage::rgen};
        desc.content = ubos[i].get();
        descriptors.push_back(desc);

        descSets[i] = vulkano.createDescriptorSet(descriptors);
    }

    vulB::VulPipeline::PipelineConfigInfo pipConf{};
    pipConf.attributeDescriptions = {{0, 0, VK_FORMAT_R32G32_SFLOAT, 0}, {1, 1, VK_FORMAT_R32G32_SFLOAT, 0}};
    pipConf.bindingDescriptions = {{0, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}, {1, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}};
    pipConf.colorAttachmentFormats = {vulkano.vulRenderer.getSwapChainColorFormat()};
    pipConf.setLayouts = {descSets[0]->getLayout()->getDescriptorSetLayout()};
    pipConf.enableColorBlending = false;
    pipConf.cullMode = VK_CULL_MODE_NONE;

    vul::Vulkano::RenderData renderData{};
    renderData.is3d = false;
    renderData.depthImageMode = vulB::VulRenderer::DepthImageMode::noDepthImage;
    renderData.swapChainImageMode = vulB::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent;
    renderData.sampleFromDepth = false;
    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) renderData.descriptorSets[i].push_back(descSets[i]);
    renderData.pipeline = std::make_shared<vulB::VulPipeline>(vulkano.getVulDevice(), "../bin/raytrace.vert.spv", "../bin/raytrace.frag.spv", pipConf);
    vulkano.renderDatas.push_back(renderData);

    vul::VulRtPipeline rtPipeline(vulkano.getVulDevice(), "../bin/raytrace.rgen.spv", {"../bin/raytrace.rmiss.spv"},
            {"../bin/raytrace.rchit.spv"}, {}, {"../bin/raytrace.rint.spv"}, {descSets[0]->getLayout()->getDescriptorSetLayout()});

    bool stop = false;
    bool createdCamStuff = false;
    while (!stop) {
        VkCommandBuffer commandBuffer = vulkano.startFrame();
        if (commandBuffer == nullptr) continue;

        if (!createdCamStuff) {
            /*
            RasUbo ubo{};
            ubo.viewMatrix = vulkano.camera.getView();
            ubo.projectionMatrix = vulkano.camera.getProjection();
            */
            glm::mat4 inverseProjMat = glm::inverse(vulkano.camera.getProjection());
            for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
                ubos[i]->writeData(&inverseProjMat, sizeof(inverseProjMat), offsetof(RtUbo, inverseProjectionMatrix));
            createdCamStuff = true;
        }

        if (vulkano.shouldShowGUI()) GuiStuff(vulkano);

        RtUbo ubo{};
        ubo.cameraPosition = glm::vec4(vulkano.cameraTransform.pos, 1.0f);
        ubo.inverseViewMatrix = glm::inverse(vulkano.camera.getView());
        ubos[vulkano.getFrameIdx()]->writeData(&ubo, sizeof(glm::vec4) + sizeof(glm::mat4), 0);

        std::vector<VkDescriptorSet> vkDescSets = {descSets[(vulkano.getFrameIdx()) % vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT]->getSet()};
        rtPipeline.traceRays(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height, 0, nullptr, vkDescSets, commandBuffer);

        stop = vulkano.endFrame(commandBuffer);

        if (vulkano.vulRenderer.wasSwapChainRecreated()) resizeRtImgs(vulkano, rtImgs);
    }
    vulkano.letVulkanoFinish();

    return 0;
}

#include "host_device.hpp"
#include "vul_comp_pipeline.hpp"
#include "vul_gltf_loader.hpp"
#include "vul_image.hpp"
#include "vul_swap_chain.hpp"
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <memory>
#include <resources.hpp>

#include <iostream>
#include <vul_rt_pipeline.hpp>
#include<vul_debug_tools.hpp>

#include<imgui.h>
#include <vulkan/vulkan_core.h>

void GuiStuff(vul::Vulkano &vulkano) {
    ImGui::Begin("Performance");
    ImGui::DragFloat("Max FPS", &vul::settings::maxFps, vul::settings::maxFps / 30.0f, 3.0f, 10'000.0f);
    ImGui::Text("Fps: %f\nTotal frame time: %fms\nIdle time %fms",
            1.0f / vulkano.getFrameTime(), vulkano.getFrameTime() * 1000.0f,
            vulkano.getIdleTime() * 1000.0f);
    ImGui::End();
}

float sRGBToAlbedo(float sRGB)
{
    const float prePow = (sRGB + 0.055) / 1.055;
    return pow(prePow, 2.4);
}

int main() {
    vul::settings::deviceInitConfig.enableRaytracingSupport = true;
    vul::Vulkano vulkano(2560, 1440, "Vulkano");
    vulkano.loadScene("../Models/sponzaCandles/sponzaCandles.gltf", "../Models/sponzaCandles/");
    vulkano.createSquare(0.0f, 0.0f, 1.0f, 1.0f);
    vul::settings::maxFps = 30.0f;

    for (int i = 0; i < 200; i++) {
        const vulB::GltfLoader::GltfLight light = vulkano.scene.lights[i];
        const float sourcePdf = 1.0f / float(vulkano.scene.lights.size());

        const float lightXDiff = std::max(light.position.x, 0.5f);
        const float lightYDiff = std::max(light.position.y, 0.5f);
        const float lightZDiff = std::max(light.position.z, 0.5f);
        const float lightStrength = sRGBToAlbedo((light.color.x + light.color.y + light.color.z) * light.intensity / 3.0f);
        const float targetPdf = lightStrength / (lightXDiff * lightXDiff + lightYDiff * lightYDiff + lightZDiff * lightZDiff) / float(vulkano.scene.lights.size());
        const float risWeight = targetPdf / sourcePdf;
        std::cout << light.name << " " << targetPdf << " " << risWeight << "\n";
    }

    std::vector<std::unique_ptr<vulB::VulBuffer>> reservoirGrids(RESERVOIR_HISTORY_LEN);
    VkExtent3D resGridSize{};
    for (int i = 0; i < RESERVOIR_HISTORY_LEN; i++) {
        ReservoirGrid resGrid = createReservoirGrid(vulkano.scene, vulkano.getVulDevice());
        resGridSize = {resGrid.width, resGrid.height, resGrid.depth};
        reservoirGrids[i] = std::move(resGrid.buffer);
    }

    std::unique_ptr<vul::VulImage> enviromentMap = vul::VulImage::createDefaultWholeImageAllInOneSingleTime(vulkano.getVulDevice(),
            "../enviromentMaps/sunsetCube.exr", {}, true, vul::VulImage::InputDataType::exrFile, vul::VulImage::ImageType::hdrCube);

    vul::VulAs as(vulkano.getVulDevice(), vulkano.scene);
    std::array<std::unique_ptr<vul::VulImage>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> rtImgs;
    std::array<std::unique_ptr<vulB::VulBuffer>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
    std::array<std::unique_ptr<vul::VulImage>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> hitCaches;
    std::array<std::shared_ptr<vulB::VulDescriptorSet>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> descSets;
    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        rtImgs[i] = std::make_unique<vul::VulImage>(vulkano.getVulDevice());
        rtImgs[i]->keepRegularRaw2d32bitRgbaEmpty(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height);
        rtImgs[i]->createDefaultImageSingleTime(vul::VulImage::ImageType::storage2d);

        ubos[i] = std::make_unique<vulB::VulBuffer>(vulkano.getVulDevice());
        ubos[i]->keepEmpty(sizeof(GlobalUbo), 1);
        ubos[i]->createBuffer(false, vulB::VulBuffer::usage_ubo);

        hitCaches[i] = std::make_unique<vul::VulImage>(vulkano.getVulDevice());
        hitCaches[i]->keepEmpty(resGridSize.width, resGridSize.height, resGridSize.depth, 1, 1, VK_FORMAT_R32_UINT, 0, 0);
        hitCaches[i]->createDefaultImageSingleTime(vul::VulImage::ImageType::storage3d);

        descSets[i] = createRtDescSet(vulkano, as, rtImgs[i], ubos[i], enviromentMap, reservoirGrids, hitCaches[i]);
    }

    uint32_t frameNumber = 0;
    vul::VulCompPipeline reservoirGridGenerator("../bin/constructReservoirGrid.comp.spv", {descSets[0]->getLayout()->getDescriptorSetLayout()}, vulkano.getVulDevice(), 1);
    reservoirGridGenerator.pPushData = &frameNumber;
    reservoirGridGenerator.pushSize = sizeof(frameNumber);

    vulkano.renderDatas.push_back(createRenderData(vulkano, vulkano.getVulDevice(), descSets));
    vul::VulRtPipeline rtPipeline(vulkano.getVulDevice(), "../bin/raytrace.rgen.spv", {"../bin/raytrace.rmiss.spv", "../bin/raytraceShadow.rmiss.spv"},
            {"../bin/raytrace.rchit.spv"}, {"../bin/raytraceShadow.rahit.spv"},
            {vulkano.renderDatas[0].descriptorSets[0][0]->getLayout()->getDescriptorSetLayout()});


    bool stop = false;
    while (!stop) {
        VkCommandBuffer commandBuffer = vulkano.startFrame();
        if (commandBuffer == nullptr) continue;
        int frameIdx = vulkano.vulRenderer.getFrameIndex();

        if (vulkano.shouldShowGUI()) GuiStuff(vulkano);
        updateUbo(vulkano, ubos[frameIdx]);

        std::vector<VkDescriptorSet> vkDescSets = {vulkano.renderDatas[0].descriptorSets[frameIdx][0]->getSet()};
        
        reservoirGridGenerator.begin({vkDescSets});
        reservoirGridGenerator.dispatch(resGridSize.width, resGridSize.height, resGridSize.depth);
        reservoirGridGenerator.end(true);

        rtPipeline.traceRays(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height, sizeof(frameNumber), &frameNumber, vkDescSets, commandBuffer);
        stop = vulkano.endFrame(commandBuffer);

        if (vulkano.vulRenderer.wasSwapChainRecreated()) resizeRtImgs(vulkano, rtImgs);
        frameNumber++;
    }
    vulkano.letVulkanoFinish();

    return 0;
}

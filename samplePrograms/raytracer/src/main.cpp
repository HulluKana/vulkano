#include <resources.hpp>

#include <iostream>
#include <vul_rt_pipeline.hpp>
#include<vul_debug_tools.hpp>

#include<imgui.h>

void GuiStuff(vul::Vulkano &vulkano) {
    ImGui::Begin("Performance");
    ImGui::DragFloat("Max FPS", &vul::settings::maxFps, vul::settings::maxFps / 30.0f, 3.0f, 10'000.0f);
    ImGui::Text("Fps: %f\nTotal frame time: %fms\nIdle time %fms",
            1.0f / vulkano.getFrameTime(), vulkano.getFrameTime() * 1000.0f,
            vulkano.getIdleTime() * 1000.0f);
    ImGui::End();
}

int main() {
    vul::settings::deviceInitConfig.enableRaytracingSupport = true;
    vul::Vulkano vulkano(2560, 1440, "Vulkano");
    vulkano.loadScene("../Models/sponzaCandles/sponzaCandles.gltf", "../Models/sponzaCandles/");
    vulkano.createSquare(0.0f, 0.0f, 1.0f, 1.0f);
    vul::settings::maxFps = 60.0f;

    ReservoirGrid reservoirGrid = createReservoirGrid(vulkano.scene, vulkano.getVulDevice());

    std::unique_ptr<vul::VulImage> enviromentMap = vul::VulImage::createDefaultWholeImageAllInOneSingleTime(vulkano.getVulDevice(),
            "../enviromentMaps/sunsetCube.exr", {}, true, vul::VulImage::InputDataType::exrFile, vul::VulImage::ImageType::hdrCube);

    vul::VulAs as(vulkano.getVulDevice(), vulkano.scene);
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

        descSets[i] = createRtDescSet(vulkano, as, rtImgs[i], ubos[i], enviromentMap, reservoirGrid.buffer);
    }

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

        std::vector<VkDescriptorSet> descSets = {vulkano.renderDatas[0].descriptorSets[(frameIdx + 1) % vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT]
            [0]->getSet()};
        rtPipeline.traceRays(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height, 0, nullptr, descSets, commandBuffer);
        stop = vulkano.endFrame(commandBuffer);

        if (vulkano.vulRenderer.wasSwapChainRecreated()) resizeRtImgs(vulkano, rtImgs);
    }
    vulkano.letVulkanoFinish();

    return 0;
}

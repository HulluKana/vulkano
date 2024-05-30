#include "rasterizing.hpp"
#include<vulkano_program.hpp>
#include <host_device.hpp>
#include <raytracing.hpp>

#include<imgui.h>
#include <iostream>

void GuiStuff(vul::Vulkano &vulkano, bool &rasterize) {
    ImGui::Begin("Performance");
    ImGui::DragFloat("Max FPS", &vul::settings::maxFps, vul::settings::maxFps / 30.0f, 3.0f, 10'000.0f);
    ImGui::Text("Fps: %f\nTotal frame time: %fms\nIdle time %fms",
            1.0f / vulkano.getFrameTime(), vulkano.getFrameTime() * 1000.0f,
            vulkano.getIdleTime() * 1000.0f);
    ImGui::Checkbox("Rasterize", &rasterize);
    ImGui::End();
}

int main() {
    vul::settings::deviceInitConfig.enableRaytracingSupport = true;
    vul::Vulkano vulkano(2560, 1440, "Vulkano");
    vul::settings::maxFps = 60.0f;
    vul::settings::cameraProperties.farPlane = 500.0f;
    vulkano.cameraController.baseMoveSpeed *= 2.5f;
    vulkano.cameraController.speedChanger *= 5.0f;

    RtResources rtRes = createRaytracingResources(vulkano);
    RasResources rasRes = createRasterizationResources(vulkano);

    bool stop = false;
    bool rasterize = false;
    bool fullUpdate = true;
    while (!stop) {
        VkCommandBuffer commandBuffer = vulkano.startFrame();
        if (commandBuffer == nullptr) continue;

        bool prevRasterize = rasterize;
        if (vulkano.shouldShowGUI()) GuiStuff(vulkano, rasterize);
        if (prevRasterize != rasterize) fullUpdate = true;

        if (rasterize) updateRasUbo(vulkano, rasRes, fullUpdate);
        else updateRtUbo(vulkano, rtRes, fullUpdate);

        if (!rasterize) raytrace(vulkano, rtRes, commandBuffer);
        vulkano.renderDatas[0].enable = !rasterize;
        vulkano.renderDatas[1].enable = rasterize;
        stop = vulkano.endFrame(commandBuffer);

        if ((vulkano.vulRenderer.wasSwapChainRecreated() && !rasterize) || fullUpdate) resizeRtImgs(vulkano, rtRes);
        fullUpdate = false;
    }
    vulkano.letVulkanoFinish();

    return 0;
}

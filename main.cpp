#include"vulkano/vulkano_program.hpp"

#include"3rdParty/imgui/imgui.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include<iostream>
#include <memory>

void GuiStuff(vul::Vulkano &vulkano, float ownStuffTime)
{   
    ImGui::Begin("Performance");
    ImGui::DragFloat("Max FPS", &vul::settings::maxFps, vul::settings::maxFps / 30.0f, 3.0f, 10'000.0f);
    ImGui::Text("Fps: %f\nTotal frame time: %fms\nObject render time: %fms\nGUI render time: %fms\nRender preparation time: %fms\nRender finishing time: %fms\nIdle time %fms\nOwn stuff: %fms",
                1.0f / vulkano.getFrameTime(), vulkano.getFrameTime() * 1000.0f, vulkano.getObjRenderTime() * 1000.0f, vulkano.getGuiRenderTime() * 1000.0f,
                vulkano.getRenderPreparationTime() * 1000.0f, vulkano.getRenderFinishingTime() * 1000.0f, vulkano.getIdleTime() * 1000.0f, ownStuffTime * 1000.0f);
    ImGui::End();
}

int main()
{
    vul::Vulkano vulkano(2560, 1440, "Vulkano");
    vulkano.loadScene("../Models/Room.glb");
    vulkano.initVulkano();
    constexpr double aspect = 480.0f / 360.0f;
    vul::settings::renderHeight = vulkano.getSwapChainExtent().height;
    vul::settings::renderWidth = std::min(static_cast<uint32_t>(vul::settings::renderHeight * aspect), vulkano.getSwapChainExtent().width);
    vul::settings::batchRender2Ds = true;

    vulkano.createNewRenderSystem({vulkano.getGlobalSetLayout()}, true, "../bin/circle.vert.spv", "../bin/circle.frag.spv");

    struct CirclePC{
        float x = 0.0f;
        float y = 0.0f;
        float radius = 1.0f;
    };
    const double radius = 1.0;
    const double cornerX = radius * sqrt(3.0);
    std::array<CirclePC, 500> circles;
    for (int i = 0; i < 500; i++){
        vulkano.createTriangle({-cornerX, radius}, {cornerX, radius}, {0.0, -radius * 2.0});
        vulkano.object2Ds[i].pCustomPushData = &circles[i];
        vulkano.object2Ds[i].customPushDataSize = sizeof(circles[i]);
    }

    bool stop = false;
    float ownStuffTime = 0.0f;
    while (!stop){
        VkCommandBuffer commandBuffer = vulkano.startFrame();
        double ownStuffStartTime = glfwGetTime();

        if (vulkano.windowWasResized()){
            vul::settings::renderHeight = vulkano.getSwapChainExtent().height;
            vul::settings::renderWidth = std::min(static_cast<uint32_t>(vul::settings::renderHeight * aspect), vulkano.getSwapChainExtent().width);
        }

        if (vulkano.shouldShowGUI()) GuiStuff(vulkano, ownStuffTime);

        ownStuffTime = glfwGetTime() - ownStuffStartTime;
        stop = vulkano.endFrame(commandBuffer);
    }

    vulkano.letVulkanoFinish();

    return 0;
}

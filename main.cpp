#include "vulkano/Backend/Headers/vul_settings.hpp"
#include"vulkano/vulkano_program.hpp"
#include"vulkano/vulkano_GUI_tools.hpp"

#include"3rdParty/imgui/imgui.h"

#include <cmath>
#include<iostream>

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

    const double radius = 1.0;
    const double cornerX = radius * sqrt(3.0);
    vulkano.createTriangle({-cornerX, radius}, {cornerX, radius}, {0.0, -radius * 2.0});
    vulkano.createTriangle({-cornerX, radius}, {cornerX, radius}, {0.0, -radius * 2.0});
    vulkano.createNewRenderSystem({vulkano.getGlobalSetLayout()}, true, "../bin/circle.vert.spv", "../bin/circle.frag.spv");

    struct CirclePC{
        float x;
        float y;
        float radius;
    };

    CirclePC circ1;
    circ1.x = -0.3f;
    circ1.y = -0.6f;
    circ1.radius = 0.15f;
    CirclePC circ2;
    circ2.x = 0.0f;
    circ2.y = 0.0f;
    circ2.radius = 0.5f;

    vulkano.object2Ds[0].pCustomPushData = &circ1;
    vulkano.object2Ds[0].customPushDataSize = sizeof(circ1);
    vulkano.object2Ds[1].pCustomPushData = &circ2;
    vulkano.object2Ds[1].customPushDataSize = sizeof(circ2);

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

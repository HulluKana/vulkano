#include<vulkano_program.hpp>
#include<host_device.hpp>
#include<vul_debug_tools.hpp>
#include <vul_acceleration_structure.hpp>

#include<imgui.h>
#include <iostream>

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
    vulkano.loadScene("../Models/Room.gltf");
    vulkano.initVulkano();
    vul::settings::maxFps = 60.0f;

    vul::VulAs as(vulkano.getVulDevice());
    as.loadScene(vulkano.scene);

    bool stop = false;
    float ownStuffTime = 0.0f;
    while (!stop) {
        VkCommandBuffer commandBuffer = vulkano.startFrame();
        if (commandBuffer == nullptr) continue;
        double ownStuffStartTime = glfwGetTime();

        if (vulkano.shouldShowGUI()) GuiStuff(vulkano, ownStuffTime);

        ownStuffTime = glfwGetTime() - ownStuffStartTime;
        stop = vulkano.endFrame(commandBuffer);
    }
    vulkano.letVulkanoFinish();

    return 0;
}

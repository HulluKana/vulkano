#include "vulkano/vulkano_program.hpp"
#include "vulkano/vulkano_defaults.hpp"

#include "3rdParty/imgui/imgui.h"

void GuiStuff(vul::Vulkano &vulkano, float ownStuffTime) {
    ImGui::Begin("Performance");
    ImGui::DragFloat("Max FPS", &vul::settings::maxFps, vul::settings::maxFps / 30.0f, 3.0f, 10'000.0f);
    ImGui::Text("Fps: %f\nTotal frame time: %fms\nObject render time: %fms\nGUI "
            "render time: %fms\nRender preparation time: %fms\nRender "
            "finishing time: %fms\nIdle time %fms\nOwn stuff: %fms",
            1.0f / vulkano.getFrameTime(), vulkano.getFrameTime() * 1000.0f,
            vulkano.getObjRenderTime() * 1000.0f,
            vulkano.getGuiRenderTime() * 1000.0f,
            vulkano.getRenderPreparationTime() * 1000.0f,
            vulkano.getRenderFinishingTime() * 1000.0f,
            vulkano.getIdleTime() * 1000.0f, ownStuffTime * 1000.0f);
    ImGui::End();
}

int main() {
    vul::Vulkano vulkano(2560, 1440, "Vulkano");
    vulkano.loadScene("../Models/Room.glb");
    for (size_t i = 0; i < vulkano.scene.images.size(); i++) vulkano.images[i + vulkano.imageCount] = vulkano.scene.images[i];
    vul::defaults::Default3dInputData default3dInputData = vul::defaults::createDefault3dInputData(vulkano);
    vul::defaults::createDefaultDescriptors(vulkano, default3dInputData);
    vul::defaults::createDefault3dRenderSystem(vulkano);
    vulkano.initVulkano();
    vul::settings::maxFps = 60.0f;

    bool stop = false;
    float ownStuffTime = 0.0f;
    while (!stop) {
        VkCommandBuffer commandBuffer = vulkano.startFrame();
        double ownStuffStartTime = glfwGetTime();

        vul::defaults::updateDefault3dInputValues(vulkano);
        if (vulkano.shouldShowGUI()) GuiStuff(vulkano, ownStuffTime);

        ownStuffTime = glfwGetTime() - ownStuffStartTime;
        stop = vulkano.endFrame(commandBuffer);
    }
    vulkano.letVulkanoFinish();

    return 0;
}
